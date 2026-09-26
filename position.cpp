#include "global_header.h"
#include "support_files.h"    // Chess, MoveList, Undo; decls for the 5 + generate_moves
#include "global_constants.h"
#include "piece_table.h"
#include "tt.h" // zobrist (incremental hash updates)
#include "tables.h" // Lookup, Move
#include "movegen.h"
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null
#include "search.h"

void make_new_state (Chess & board, uint16_t move) {

    auto & bb = board.bitboard;
    auto & mb = board.mailbox;
    int from = _pext_u32(move, from_square_mask);
    int to = _pext_u32(move, to_square_mask);
    int moved    = board.mailbox[from];              // original moving piece
    int captured = board.mailbox[to];                // original piece on 'to' (0 if none)
    uint64_t status_before = board.bitboard[status]; // status word before the move
    int pit = board.pit;
    uint64_t changes_in_state {0}, ev_bit;
    int number_of_changes, ev_square;
    int halfmove_clock = _pext_u64(board.bitboard[status], halfmove_clock_mask); // bit 25-31 in [status] 

    bb[status] &= ~my_const::row_6_mask & ~my_const::row_3_mask; // clear old e.p. targets
    halfmove_clock = ((move & 15) == 0 and mb[from] != Pawn) ? halfmove_clock + 1 : 0;

    switch (move & 15) {

        case 0: {  // quiet move
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[mb[from]] |= (1ULL << to);
            mb[to] = mb[from];
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][moved][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][moved][pit]; // put in
        } break;

        case 1: {  // double pawn push
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[mb[from]] |= (1ULL << to);
            mb[to] = mb[from];
            mb[from] = 0;
            bb[status] |= ((((1ULL << to) >> 8) & my_const::pit_on_off [!!(bb[Pawn] & bb[!pit] & my_const::en_passant[to])] & my_const::pit_on_off[!pit]) | 
                          (((1ULL << to) << 8) & my_const::pit_on_off [!!(bb[Pawn] & bb[!pit] & my_const::en_passant[to])] & my_const::pit_on_off[pit]));
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][moved][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][moved][pit]; // put in
        } break;

        case 2: {  // king side castle
            uint64_t king_squares = ((e1 | g1) & my_const::pit_on_off[!pit]) | ((e8 | g8) & my_const::pit_on_off[pit]);
            uint64_t rook_squares = ((f1 | h1) & my_const::pit_on_off[!pit]) | ((f8 | h8) & my_const::pit_on_off[pit]);
            bb[7] ^= king_squares;
            bb[5] ^= rook_squares;
            bb[pit] ^= king_squares | rook_squares;
            mb[4 + 56 * pit] = 0;
            mb[7 + 56 * pit] = 0;
            mb[5 + 56 * pit] = 5;
            mb[6 + 56 * pit] = 7;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[7 + 56 * pit][Rook][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[5 + 56 * pit][Rook][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[4 + 56 * pit][King][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[6 + 56 * pit][King][pit]; // put in
        } break;

        case 3: {   // queen side castle
            uint64_t king_squares = ((e1 | c1) & my_const::pit_on_off[!pit]) | ((e8 | c8) & my_const::pit_on_off[pit]);
            uint64_t rook_squares = ((a1 | d1) & my_const::pit_on_off[!pit]) | ((a8 | d8) & my_const::pit_on_off[pit]);
            bb[7] ^= king_squares;
            bb[5] ^= rook_squares;
            bb[pit] ^= king_squares | rook_squares;
            mb[4 + 56 * pit] = 0;
            mb[0 + 56 * pit] = 0;
            mb[3 + 56 * pit] = 5;
            mb[2 + 56 * pit] = 7;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[0 + 56 * pit][Rook][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[3 + 56 * pit][Rook][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[4 + 56 * pit][King][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[2 + 56 * pit][King][pit]; // put in
        } break;

        case 4: {  // capture 
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << to);
            bb[mb[to]] &= ~(1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[mb[from]] |= (1ULL << to);
            mb[to] = mb[from];
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][moved][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][moved][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][captured][!pit]; // take out opposition
        } break;
        
        case 5: {  // capture en passant
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << (to - 8 + 16 * pit));
            bb[Pawn] &= ~(1ULL << (to - 8 + 16 * pit));
            bb[Pawn] &= ~(1ULL << from);
            bb[Pawn] |= (1ULL << to);
            mb[to] = mb[from];
            mb[from] = 0;
            mb[to - 8 + 16 * pit] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Pawn][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to - 8 + 16 * pit][Pawn][!pit]; // take out opposition
        } break;

        case 8: {  // knight promotion
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[Pawn] &= ~(1ULL << from);
            bb[Knight] |= (1ULL << to);
            mb[to] = Knight;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Knight][pit]; // put in promotion
        } break;

        case 9: {  // bishop promotion
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[Pawn] &= ~(1ULL << from);
            bb[Bishop] |= (1ULL << to);
            mb[to] = Bishop;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Bishop][pit]; // put in promotion
        } break;
        
        case 10: {  // rook promotion
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[Pawn] &= ~(1ULL << from);
            bb[Rook] |= (1ULL << to);
            mb[to] = Rook;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Rook][pit]; // put in promotion
        } break;

        case 11: {  // queen promotion
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[Pawn] &= ~(1ULL << from);
            bb[Queen] |= (1ULL << to);
            mb[to] = Queen;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Queen][pit]; // put in promotion
        } break;

        case 12: {  // knight promotion capture 
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << to);
            bb[mb[to]] &= ~(1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[Knight] |= (1ULL << to);
            mb[to] = Knight;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Knight][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][captured][!pit]; // take out opposition
        } break;

        case 13: {  // bishop promotion capture 
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << to);
            bb[mb[to]] &= ~(1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[Bishop] |= (1ULL << to);
            mb[to] = Bishop;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Bishop][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][captured][!pit]; // take out opposition
        } break;

        case 14: {  // rook promotion capture 
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << to);
            bb[mb[to]] &= ~(1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[Rook] |= (1ULL << to);
            mb[to] = Rook;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Rook][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][captured][!pit]; // take out opposition
        } break;

        case 15: {  // queen promotion capture 
            bb[pit] &= ~(1ULL << from);
            bb[pit] |= (1ULL << to);
            bb[!pit] &= ~(1ULL << to);
            bb[mb[to]] &= ~(1ULL << to);
            bb[mb[from]] &= ~(1ULL << from);
            bb[Queen] |= (1ULL << to);
            mb[to] = Queen;
            mb[from] = 0;
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[from][Pawn][pit]; // take out
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][Queen][pit]; // put in
            board.tpt.zobrist_key_64 ^= zobrist.hash_piece[to][captured][!pit]; // take out opposition
        } break;
    };

    // update castling right status - updates the start value of bb[status] bit 0, 7, 56, and 63 - a set bit indicates piece in original position
    bb[status] &= (a1 * (!!(a1 & bb[Rook]) and !!(e1 & bb[King]))) | (h1 * (!!(h1 & bb[Rook]) and !!(e1 & bb[King]))) | 
                     (a8 * (!!(a8 & bb[Rook]) and !!(e8 & bb[King]))) | (h8 * (!!(h8 & bb[Rook]) and !!(e8 & bb[King]))) | 
                     my_const::row_3_mask | my_const::row_6_mask | my_const::row_4_mask;

    bb[status] &= ~check_flag; // clear the in-check bit (24); generate_moves sets it again for the new position
    pit ^= 1U; // this should reverse the player in turn (pit) status
    board.tpt.zobrist_key_64 ^= zobrist.hash_piece[0][0][1]; // reverse pit
    
    bb[status] &= ~halfmove_clock_mask; // clear old clock (bits 25-31)
    bb[status] |= _pdep_u64(halfmove_clock, halfmove_clock_mask); // store the half-move clock in status bits 25-31

    board.pit = pit;
    board.possible_moves.clear();
    board.possible_captures.clear();

    // TranspositionTable updates in [state] - castling rights and e.p. targets
    changes_in_state = (status_before ^ bb[status]) & ep_castling_mask;
    number_of_changes = std::popcount(changes_in_state);
    for (int i = 0; i < number_of_changes; ++i) {
        ev_bit = changes_in_state & (~changes_in_state + 1ULL); // take the rightmost bit in changes_in_state and make it "evaluation_bit"
        changes_in_state &= ~ev_bit; // remove the "evaluation_bit" from changes_in_state
        ev_square = std::countr_zero(ev_bit);
        board.tpt.zobrist_key_64 ^= zobrist.hash_piece[ev_square][0][0];
    }
}

// ===================== make / unmake (in-place) =====================
// make: save the "undo ticket" (the info the move destroys), then apply the move with make_new_state.
void make (Chess & board, uint16_t move, Undo & undo) {
    int to = _pext_u32(move, to_square_mask);
    undo.old_status = board.bitboard[status];        // castling + ep + check + clock, one word
    undo.old_key    = board.tpt.zobrist_key_64;
    undo.captured   = board.mailbox[to];             // 0 for quiet / e.p. (e.p. pawn isn't on 'to')
    make_new_state(board, move);
}

// unmake: reverse ONLY the piece placement (mirror of make_new_state's switch); everything
// else (castling / e.p. / clock / key) is restored wholesale from the saved ticket.
void unmake (Chess & board, uint16_t move, const Undo & undo) {
    board.pit ^= 1U;                                 // side to move goes back to the mover
    int mp = board.pit;                              // colour that made the move
    int op = mp ^ 1;                                 // opponent (the captured side)
    int from = _pext_u32(move, from_square_mask);
    int to   = _pext_u32(move, to_square_mask);
    auto & bb = board.bitboard;
    auto & mb = board.mailbox;

    switch (move & 15) {

        case 0:                                      // quiet
        case 1: {                                    // double push (ep bit comes back via old_status)
            int pc = mb[to];
            bb[mp] &= ~(1ULL << to); bb[mp] |= (1ULL << from);
            bb[pc] &= ~(1ULL << to); bb[pc] |= (1ULL << from);
            mb[from] = pc; mb[to] = 0;
        } break;

        case 4: {                                    // capture
            int pc = mb[to];
            bb[mp] &= ~(1ULL << to); bb[mp] |= (1ULL << from);
            bb[pc] &= ~(1ULL << to); bb[pc] |= (1ULL << from);
            mb[from] = pc;
            int cp = undo.captured;                  // put the captured piece back on 'to'
            bb[op] |= (1ULL << to); bb[cp] |= (1ULL << to);
            mb[to] = cp;
        } break;

        case 2: {                                    // king-side castle (XOR is self-inverse)
            uint64_t king_squares = ((e1 | g1) & my_const::pit_on_off[!mp]) | ((e8 | g8) & my_const::pit_on_off[mp]);
            uint64_t rook_squares = ((f1 | h1) & my_const::pit_on_off[!mp]) | ((f8 | h8) & my_const::pit_on_off[mp]);
            bb[7] ^= king_squares; bb[5] ^= rook_squares; bb[mp] ^= king_squares | rook_squares;
            mb[4 + 56 * mp] = King; mb[7 + 56 * mp] = Rook; mb[5 + 56 * mp] = 0; mb[6 + 56 * mp] = 0;
        } break;

        case 3: {                                    // queen-side castle
            uint64_t king_squares = ((e1 | c1) & my_const::pit_on_off[!mp]) | ((e8 | c8) & my_const::pit_on_off[mp]);
            uint64_t rook_squares = ((a1 | d1) & my_const::pit_on_off[!mp]) | ((a8 | d8) & my_const::pit_on_off[mp]);
            bb[7] ^= king_squares; bb[5] ^= rook_squares; bb[mp] ^= king_squares | rook_squares;
            mb[4 + 56 * mp] = King; mb[0 + 56 * mp] = Rook; mb[3 + 56 * mp] = 0; mb[2 + 56 * mp] = 0;
        } break;

        case 5: {                                    // en-passant capture
            int cap_sq = to - 8 + 16 * mp;
            bb[mp]   &= ~(1ULL << to); bb[mp]   |= (1ULL << from);
            bb[Pawn] &= ~(1ULL << to); bb[Pawn] |= (1ULL << from);
            bb[op]   |= (1ULL << cap_sq); bb[Pawn] |= (1ULL << cap_sq);   // captured pawn back
            mb[from] = Pawn; mb[to] = 0; mb[cap_sq] = Pawn;
        } break;

        case 8: case 9: case 10: case 11: {          // promotion (no capture)
            int promo = mb[to];
            bb[mp]    &= ~(1ULL << to); bb[mp] |= (1ULL << from);
            bb[promo] &= ~(1ULL << to);
            bb[Pawn]  |= (1ULL << from);
            mb[from] = Pawn; mb[to] = 0;
        } break;

        case 12: case 13: case 14: case 15: {        // promotion with capture
            int promo = mb[to];
            bb[mp]    &= ~(1ULL << to); bb[mp] |= (1ULL << from);
            bb[promo] &= ~(1ULL << to);
            bb[Pawn]  |= (1ULL << from);
            mb[from] = Pawn;
            int cp = undo.captured;
            bb[op] |= (1ULL << to); bb[cp] |= (1ULL << to);
            mb[to] = cp;
        } break;
    }

    bb[status] = undo.old_status;                    // Bucket B restored wholesale
    board.tpt.zobrist_key_64 = undo.old_key;
}

// ---------------------------------------------------------------------------------------
// Null move (a "pass") for null-move pruning. A strict subset of make_new_state: it moves no
// piece and touches no castling right or clock — it only (1) flips the side to move and (2)
// closes the e.p. window, keeping the Zobrist key consistent for both. Reuses the Undo ticket
// ({old_status, old_key}); `captured` is unused. Never push a null position to the repetition
// list — a pass must not pollute the threefold stack.
// ---------------------------------------------------------------------------------------
void make_null (Chess & board, Undo & undo) {
    auto & bb = board.bitboard;
    undo.old_status = bb[status];                    // save what a pass destroys...
    undo.old_key    = board.tpt.zobrist_key_64;
    uint64_t status_before = bb[status];

    bb[status] &= ~my_const::row_6_mask & ~my_const::row_3_mask;   // a pass closes the e.p. window

    board.pit ^= 1U;                                              // flip side to move...
    board.tpt.zobrist_key_64 ^= zobrist.hash_piece[0][0][1];      // ...and its key term

    // fold the cleared e.p. squares out of the key (only e.p. bits changed — castling untouched)
    uint64_t changes = (status_before ^ bb[status]) & ep_castling_mask;
    while (changes) {
        int sq = std::countr_zero(changes);
        changes &= changes - 1;                                   // clear the lowest set bit
        board.tpt.zobrist_key_64 ^= zobrist.hash_piece[sq][0][0];
    }
}

void unmake_null (Chess & board, const Undo & undo) {
    board.pit ^= 1U;                                 // side to move goes back
    board.bitboard[status]   = undo.old_status;      // e.p. window (+ clock) restored wholesale
    board.tpt.zobrist_key_64 = undo.old_key;
}