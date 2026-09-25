#include "global_header.h"
#include "support_files.h"
#include "global_constants.h"
#include "piece_table.h"
#include "tt.h" // zobrist
#include "tables.h" // Lookup, Move
#include "movegen.h"
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null

void generate_moves(Chess & Board) { 
    
    // variable declaration for this function
    uint64_t piece_bb, ev_bit, ev_bit_2, from, move_to_bits, en_passant_bit, under_attack_bb = 0UL, king_rays, 
             pinners, pinned_piece, diag_pinned_bb {~0UL}, r_f_pinned_bb {~0UL}, r_f_pinned_pawn_bb {~0UL};
    int ev_square, ev_square_2;
    std::array<int, 65> diag_pinned_pointer; // NOT filled — see pinned_any below
    std::array<int, 65> r_f_pinned_pointer; 
    bool King_in_Check {false};

    // these variables is used for readability of code and convenience  - local copies.... 
    bool pit = Board.pit; // pit is false (0) when white is in play and true (1) when black is in play
    MoveList move_list, capture_list, sorted_captures; // fixed-size, on the stack

    // this is done so many times - so let's save some typing time.....
    uint64_t all_pop = Board.bitboard[White] | Board.bitboard[Black];
    uint64_t no_pop = ~all_pop;

    // generate a bitboard "under_attack_bb" for all positions under attack from opposition 
    // attacked by Bishops and Queens 
    piece_bb =  Board.bitboard[!pit] & (Board.bitboard[Bishop] | Board.bitboard[Queen]);
    while (piece_bb) {
        ev_square = __builtin_ctzll(piece_bb); 
        under_attack_bb |= Lookup.diag.attacks(ev_square, (Board.bitboard[pit] & ~Board.bitboard[King]) | Board.bitboard[!pit]);
        piece_bb &= piece_bb - 1UL;
    }

    // attacked by Rooks and Queens 
    piece_bb =  Board.bitboard[!pit] & (Board.bitboard[Rook] | Board.bitboard[Queen]);
    while (piece_bb) { // loop all opposition Bishops and Queens on the board
        ev_square = __builtin_ctzll(piece_bb); 
        under_attack_bb |= Lookup.rank_file.attacks(ev_square, (Board.bitboard[pit] & ~Board.bitboard[King]) | Board.bitboard[!pit]);
        piece_bb &= piece_bb - 1UL;
    }

    // attacked by Knights
    piece_bb = Board.bitboard[!pit] & Board.bitboard[Knight];
    while (piece_bb) {  // loop all Knights on the board
        ev_square = __builtin_ctzll(piece_bb); 
        under_attack_bb |= my_const::knight_neighbors[ev_square];
        piece_bb &= piece_bb - 1UL;
    }

    // attacked by Pawns
    piece_bb = Board.bitboard[!pit] & Board.bitboard[Pawn];
    while (piece_bb) {  // loop all Knights on the board
        ev_square = __builtin_ctzll(piece_bb); 
        under_attack_bb |= my_const::pawn_neighbors[!pit][ev_square];
        piece_bb &= piece_bb - 1UL;
    }

    // attacked by King
    piece_bb =  Board.bitboard[!pit] & Board.bitboard[King]; // there will be only one king - make it "evaluation_bit"
    under_attack_bb |= my_const::king_neighbors[__builtin_ctzll(piece_bb)];
    King_in_Check = Board.bitboard[pit] & Board.bitboard[King] & under_attack_bb; // flag "King_in_Check" is set when pit king is under attack

    // pinned pieces analysis

    // find the space from the king perspective as if it was a slider - only own blockers !!! - king_rays
    // find the sliders that are on the same diag as the king - limit the population to only opposition sliders and dismiss other hits
    // loop the found pinners and generate their action area (same as move_to) - pinners_rays
    // then pinned pieces are found with pinners_rays (internal) AND kings_rays
    ev_bit =  Board.bitboard[pit] & Board.bitboard[King]; // there will be only one king - make it "evaluation_bit" 
    ev_square = __builtin_ctzll(ev_bit); //  
    king_rays = ~Board.bitboard[!pit] & Lookup.diag.attacks(ev_square, all_pop);
    pinners = Board.bitboard[!pit] & (Board.bitboard[Bishop] | Board.bitboard[Queen]) & Lookup.diag.attacks(ev_square, 0UL);
    while (pinners) {  // loop all diag pinners - max 4 of them 
        ev_bit_2 = pinners & (~pinners + 1UL); // take the rightmost bit in diag_pinners and make it a new "evaluation_bit_2"
        pinners &= pinners - 1UL; // remove the "evaluation_bit_2" from diag_pinners 
        ev_square_2 = __builtin_ctzll(ev_bit_2); 
        // pinned pieces within one pinners rays and the king rays
        pinned_piece = king_rays & (~Board.bitboard[!pit] & Lookup.diag.attacks(ev_square_2, all_pop));
        if (!pinned_piece) continue;   // 2+ pieces between king and slider: rays never meet, no pin (and ctz(0) is UB)
        // turn pinned_piece into a square - find the common diagonal beween pinner and the kings square -- keep in a matrix [pinned_piece] of 64 - use in generation
        diag_pinned_pointer[__builtin_ctzll(pinned_piece)] = my_const::diagonal_pointer[ev_square_2][0] * (my_const::diagonal_pointer[ev_square][0] == my_const::diagonal_pointer[ev_square_2][0]) + 
                                                        my_const::diagonal_pointer[ev_square_2][1] * (my_const::diagonal_pointer[ev_square][0] != my_const::diagonal_pointer[ev_square_2][0]);
        diag_pinned_bb &= ~pinned_piece;
        r_f_pinned_pointer[__builtin_ctzll(pinned_piece)] = 17; // when pinned diagonal - set pinned rank/file to return 0 since both cannot exist in parallell
    }
    
    king_rays = ~Board.bitboard[!pit] & Lookup.rank_file.attacks(ev_square, all_pop);
    pinners = Board.bitboard[!pit] & (Board.bitboard[Rook] | Board.bitboard[Queen]) & Lookup.rank_file.attacks(ev_square, 0UL);
    while (pinners) {  // loop all diag pinners - max 4 of them 
        ev_bit_2 = pinners & (~pinners + 1UL); // take the rightmost bit in diag_pinners and make it a new "evaluation_bit_2"
        pinners &= pinners - 1UL;// remove the "evaluation_bit_2" from diag_pinners 
        ev_square_2 = __builtin_ctzll(ev_bit_2); 
        // pinned pieces within one pinners rays and the king rays
        pinned_piece = king_rays & (~Board.bitboard[!pit] & Lookup.rank_file.attacks(ev_square_2, all_pop));
        if (!pinned_piece) continue;   // 2+ pieces between king and slider: rays never meet, no pin (and ctz(0) is UB)
        int pinned_piece_square = __builtin_ctzll(pinned_piece);
        // turn pinned_piece into a square - find the diagonal beween pinner and the kings square -- keep in a matrix [pinned_piece] of 64 - use in generation
        r_f_pinned_pointer[pinned_piece_square] = my_const::ranks_and_files_pointer[ev_square_2][0] * (my_const::ranks_and_files_pointer[ev_square][0] == my_const::ranks_and_files_pointer[ev_square_2][0]) + 
                                                    my_const::ranks_and_files_pointer[ev_square_2][1] * (my_const::ranks_and_files_pointer[ev_square][0] != my_const::ranks_and_files_pointer[ev_square_2][0]);
        r_f_pinned_bb &= ~pinned_piece;
        r_f_pinned_pawn_bb ^= pinned_piece * !(((my_const::ranks_and_files[r_f_pinned_pointer[pinned_piece_square]]) & my_const::pawn_movespace[pit][pinned_piece_square]) == 
                                 my_const::pawn_movespace[pit][pinned_piece_square]); 
        diag_pinned_pointer[pinned_piece_square] = 27; // when pinned r/f - set pinned diagonal to return 0 since both cannot exist in parallell
    }

    // Only pinned squares ever got a table entry written; every other square is unrestricted,
    // i.e. exactly diagonals[26] / ranks_and_files[16] == ~0UL. So the two 65-entry .fill()s
    // (520 B of stores on EVERY node, for tables that are almost always entirely default) are
    // not needed at all — guard the six read sites instead.
    uint64_t pinned_any = ~(diag_pinned_bb & r_f_pinned_bb);
    auto dmask = [&](uint64_t bit, int sq) { return (bit & pinned_any) ? my_const::diagonals[diag_pinned_pointer[sq]] : ~0UL; };
    auto rmask = [&](uint64_t bit, int sq) { return (bit & pinned_any) ? my_const::ranks_and_files[r_f_pinned_pointer[sq]] : ~0UL; };

    // generates moves for pawns (only simple move forward including promotion)
    piece_bb =  (((((Board.bitboard[pit] & Board.bitboard[Pawn] & diag_pinned_bb & r_f_pinned_pawn_bb) << 8) & no_pop) & my_const::pit_on_off[!pit]) |
                    ((((Board.bitboard[pit] & Board.bitboard[Pawn] & diag_pinned_bb & r_f_pinned_pawn_bb) >> 8) & no_pop) & my_const::pit_on_off[pit]));
    while (piece_bb) {
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in piece_bb and make it the "evaluation_bit"
        piece_bb &= piece_bb - 1UL;  // remove the "evaluation_bit" from piece_bb (remove one pawn if there is more than one)
        from = (((ev_bit >> 8) & my_const::pit_on_off[!pit]) | ((ev_bit << 8) & my_const::pit_on_off[pit]));
        if (!(ev_bit & (my_const::row_1_mask | my_const::row_8_mask))) Move.from_to (from, ev_bit, 0, move_list);
        else Move.from_to_promotion(from, ev_bit, move_list);
    }

    // generate moves for pawns (only the double opening move)
    piece_bb =  ((((Board.bitboard[pit] & Board.bitboard[Pawn] & diag_pinned_bb & r_f_pinned_pawn_bb & my_const::row_2_mask) << 16) & ((no_pop & my_const::row_3_mask) << 8) & (no_pop & my_const::row_4_mask)) & my_const::pit_on_off[!pit]) |
                    ((((Board.bitboard[pit] & Board.bitboard[Pawn] & diag_pinned_bb & r_f_pinned_pawn_bb & my_const::row_7_mask) >> 16) & ((no_pop & my_const::row_6_mask) >> 8) & (no_pop & my_const::row_5_mask)) & my_const::pit_on_off[pit]);
    while (piece_bb) {
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in set_bitboard and make it the "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one pawn if there is more than one)
        from = (((ev_bit >> 16) & my_const::pit_on_off[!pit])| ((ev_bit << 16) & my_const::pit_on_off[pit]));
        Move.from_to (from, ev_bit, 1, move_list);
    }

    // generates captures for pawn including capture and promation
    piece_bb =  Board.bitboard[pit] & Board.bitboard[Pawn] & r_f_pinned_bb;
    while (piece_bb) { // loop all pawns on the board
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in set_bitboard and make it the "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one Knight if there is more than one)
        ev_square = __builtin_ctzll(ev_bit); 
        move_to_bits = my_const::pawn_neighbors[pit][ev_square] & Board.bitboard[!pit] & ~Board.bitboard[King] & dmask(ev_bit, ev_square);
        en_passant_bit =  my_const::pawn_neighbors[pit][ev_square] & Board.bitboard[status] & (my_const::row_6_mask | my_const::row_3_mask) & dmask(ev_bit, ev_square);

        // --- en-passant horizontal (rank) pin ----------------------------------------
        // The e.p. capture yanks BOTH pawns off their shared rank in one move. If the
        // friendly king is on that rank with an enemy rook/queen beyond, that exposes
        // the king -> the capture is illegal. Ordinary pin detection misses it (two
        // pieces leave the rank at once, and the captured pawn isn't on the e.p.
        // target square).
        if (en_passant_bit) {
            uint64_t king_bit  = Board.bitboard[pit] & Board.bitboard[King];
            uint64_t rank_mask = 0xFFUL << (ev_square & 56);              // rank of the capturing pawn

            if (king_bit & rank_mask) {                                   // king shares that rank
                int      ep_sq     = __builtin_ctzll(en_passant_bit);
                uint64_t file_mask = 0x0101010101010101UL << (ep_sq & 7); // file of the e.p. target
                uint64_t captured  = Board.bitboard[!pit] & Board.bitboard[Pawn] & rank_mask & file_mask;
                uint64_t occ       = all_pop & ~ev_bit & ~captured;       // remove BOTH pawns at once
                int      king_sq   = __builtin_ctzll(king_bit);
                uint64_t seen      = Lookup.rank_file.attacks(king_sq, occ) & rank_mask;
                if (seen & Board.bitboard[!pit] & (Board.bitboard[Rook] | Board.bitboard[Queen]))
                    en_passant_bit = 0;                                   // illegal -> suppress
            }
        }
        // --------------------------------------------------------------------------
                
        // executes an en passant capture
        if (en_passant_bit)  Move.capture(ev_bit, en_passant_bit, 5, capture_list);

        // generates a capture or capture with promotion 
        if (!(move_to_bits & (my_const::row_1_mask | my_const::row_8_mask))) Move.capture(ev_bit, move_to_bits, 4, capture_list);
        else Move.capture_promotion(ev_bit, move_to_bits, capture_list);
    }

    // generates moves for knights
    piece_bb = Board.bitboard[pit] & Board.bitboard[Knight] & diag_pinned_bb & r_f_pinned_bb;
    while (piece_bb) {  // loop all Knights on the board
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in piece_bb and make it the "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one Knight if there is more than one)
        ev_square = __builtin_ctzll(ev_bit); 
        move_to_bits = my_const::knight_neighbors[ev_square] & (no_pop | (Board.bitboard[!pit] & ~Board.bitboard[King]));
        if (!move_to_bits) continue;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, capture_list);
        Move.from_to (ev_bit, no_pop & move_to_bits, 0, move_list);
    }

    // generates moves for Bishops 
    piece_bb =  Board.bitboard[pit] & Board.bitboard[Bishop];
    while (piece_bb) {  // loop all Bishops on the board (can be more than one)
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in piece_bb and make it "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one Bishop if there is more than one)
        ev_square = __builtin_ctzll(ev_bit); 
        // use PEXT instruction _pext_u64(population, blockers mask[square] - index) as index for the huge lookup table - remove own pieces last
        move_to_bits = ~Board.bitboard[pit] & ~Board.bitboard[King] & Lookup.diag.attacks(ev_square, all_pop) & dmask(ev_bit, ev_square);
        if (!move_to_bits) continue;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, capture_list);
        Move.from_to (ev_bit, no_pop & move_to_bits, 0, move_list);
    }

    // generates moves for Rooks 
    piece_bb =  Board.bitboard[pit] & Board.bitboard[Rook];
    while (piece_bb) {  // loop all Rooks on the board (can be more than one)
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in piece_bb and make it "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one Rook if there is more than one)
        ev_square = __builtin_ctzll(ev_bit); 
        move_to_bits = ~Board.bitboard[pit] & ~Board.bitboard[King] & Lookup.rank_file.attacks(ev_square, all_pop) & rmask(ev_bit, ev_square);
        if (!move_to_bits) continue;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, capture_list);
        Move.from_to (ev_bit, no_pop & move_to_bits, 0, move_list); 
    }

    // generates moves for Queens 
    piece_bb =  Board.bitboard[pit] & Board.bitboard[Queen];
    while (piece_bb) {   // loop all Rooks on the board (can be more than one)
        ev_bit = piece_bb & (~piece_bb + 1UL); // take the rightmost bit in piece_bb and make it "evaluation_bit"
        piece_bb &= piece_bb - 1UL; // remove the "evaluation_bit" from piece_bb (remove one Queen if there is more than one)
        ev_square = __builtin_ctzll(ev_bit); 
        move_to_bits = ~Board.bitboard[pit] & ~Board.bitboard[King] & ((Lookup.rank_file.attacks(ev_square, all_pop) & rmask(ev_bit, ev_square)) | 
                                      (Lookup.diag.attacks(ev_square, all_pop) & dmask(ev_bit, ev_square)));
        if (!move_to_bits) continue;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, capture_list);
        Move.from_to (ev_bit, no_pop & move_to_bits, 0, move_list); 
    }

    // generates moves for King
    ev_bit =  Board.bitboard[pit] & Board.bitboard[King]; // there will be only one king - make it "evaluation_bit"
    move_to_bits = my_const::king_neighbors[__builtin_ctzll(ev_bit)] & (no_pop | (Board.bitboard[!pit] & ~Board.bitboard[King])) & ~under_attack_bb;
    Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, capture_list);
    Move.from_to (ev_bit, no_pop & move_to_bits, 0, move_list); 

    // generates moves for Castling - look out - this is a mix of bit and logic operators !!
    if (!(white_queen_side & under_attack_bb) and (Board.bitboard[status] & a1) and !(all_pop & (b1 | c1 | d1)) and !pit) 
        Move.from_to(e1, c1, 3, move_list);
    if (!(white_king_side & under_attack_bb) and (Board.bitboard[status] & h1) and !(all_pop & (f1 | g1)) and !pit) 
        Move.from_to(e1, g1, 2, move_list);
    if (!(black_queen_side & under_attack_bb) and (Board.bitboard[status] & a8) and !(all_pop & (b8 | c8 | d8)) and pit) 
        Move.from_to(e8, c8, 3, move_list);
    if (!(black_king_side & under_attack_bb) and (Board.bitboard[status] & h8) and !(all_pop & (f8 | g8)) and pit) 
        Move.from_to(e8, g8, 2, move_list);

    Board.possible_captures = capture_list;
    Board.possible_moves = move_list;

    // Check evasion if required - this function will update the possible_moves and possible_captures lists to only legal moves if the king is in check
    bool in_check = King_in_Check; // side to move IS in check
    if (King_in_Check) King_in_Check = check_evasion (Board, under_attack_bb);  // still needed: filters to legal evasions
    Board.bitboard[status] ^= check_flag * in_check; // bit 24 = IN CHECK

    MoveList non_ordered = Board.possible_captures; // starts as the sorted captures
    for (uint16_t m : Board.possible_moves) non_ordered.push_back(m);  // then append the quiets
    Board.possible_moves = non_ordered;

}

bool check_evasion (Chess & Board, uint64_t under_attack_bb) {

    // these variables is used for readability of code and convenience  - local copies.... 
    bool pit = Board.pit, king_in_check {true}; 
    uint64_t all_pop = Board.bitboard[White] | Board.bitboard[Black];
    MoveList temp_possible_moves, temp_possible_captures;
    uint64_t diag_move_to_bits, r_f_move_to_bits, move_to_bits {0UL}, diag_opp_bit, r_f_opp_bit, temp_mask, low, high;
    int ev_checker_square;

    uint64_t ev_bit = Board.bitboard[pit] & Board.bitboard[King]; // there will be only one king - make it "evaluation_bit"
    int ev_square = __builtin_ctzl(ev_bit);
    diag_move_to_bits = ((Board.bitboard[!pit] & (Board.bitboard[Bishop] | Board.bitboard[Queen])) | ~all_pop) & 
                            Lookup.diag.attacks(ev_square, all_pop);
    
    diag_opp_bit = diag_move_to_bits & Board.bitboard[!pit]; // islolates the checker - also used later
    temp_mask = diag_opp_bit | ev_bit; // put two bits in temp mask so that we can find low and high
    low = temp_mask & (~temp_mask + 1);
    temp_mask &= ~low;
    high = temp_mask & (~temp_mask + 1);
    diag_move_to_bits &= ((high - 1UL) ^ (low - 1UL)) | high; // use the new mask to sort out and discard what is "behind the king"  

    r_f_move_to_bits = ((Board.bitboard[!pit] & (Board.bitboard[Rook] | Board.bitboard[Queen])) | ~all_pop) & 
                            Lookup.rank_file.attacks(ev_square, all_pop);

    r_f_opp_bit = r_f_move_to_bits & Board.bitboard[!pit]; // islolates the checker - also used later
    temp_mask = r_f_opp_bit | ev_bit; // put two bits in temp mask so that we can find low and high
    low = temp_mask & (~temp_mask + 1);
    temp_mask &= ~low;
    high = temp_mask & (~temp_mask + 1);
    r_f_move_to_bits &= ((high - 1UL) ^ (low - 1UL)) | high;  // use the new mask to sort out and discard what is "behind the king"                    

    move_to_bits = my_const::knight_neighbors[ev_square] & (Board.bitboard[!pit] & Board.bitboard[Knight]);
    move_to_bits |= my_const::pawn_neighbors[pit][ev_square] & (Board.bitboard[!pit] & Board.bitboard[Pawn]); // move_to_bits are moves where "to" is blocking or capturing checker

    if (__builtin_popcountl(move_to_bits | diag_opp_bit | r_f_opp_bit) > 1) { // king is in double check - moving the king is the only chance - forget all generated moves
        
        Board.possible_moves.clear();
        Board.possible_captures.clear();
        move_to_bits = my_const::king_neighbors[ev_square] & (~all_pop | Board.bitboard[!pit]) & ~under_attack_bb;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, Board.possible_captures);
        Move.from_to (ev_bit, ~all_pop & move_to_bits, 0, Board.possible_moves);
        king_in_check = (Board.possible_moves.empty() & Board.possible_captures.empty());
        return king_in_check;
    }
    else {  // king is in single check only - select the moves from the alreay generated list that blocks, captures or moves the king out of check
        ev_checker_square = _tzcnt_u64(diag_opp_bit);
        int diag_check_pointer = my_const::diagonal_pointer[ev_checker_square][0] * (my_const::diagonal_pointer[ev_square][0] == my_const::diagonal_pointer[ev_checker_square][0]) + 
                                    my_const::diagonal_pointer[ev_checker_square][1] * (my_const::diagonal_pointer[ev_square][0] != my_const::diagonal_pointer[ev_checker_square][0]);
        ev_checker_square = _tzcnt_u64(r_f_opp_bit);
        int r_f_check_pointer = my_const::ranks_and_files_pointer[ev_checker_square][0] * (my_const::ranks_and_files_pointer[ev_square][0] == my_const::ranks_and_files_pointer[ev_checker_square][0]) + 
                                    my_const::ranks_and_files_pointer[ev_checker_square][1] * (my_const::ranks_and_files_pointer[ev_square][0] != my_const::ranks_and_files_pointer[ev_checker_square][0]);
        move_to_bits |= ((diag_move_to_bits & my_const::diagonals[diag_check_pointer]) | (r_f_move_to_bits & my_const::ranks_and_files[r_f_check_pointer]));
        // en-passant evasion: if the checker is a pawn that just double-pushed, the e.p.
        // capture removes it but LANDS on the e.p. square (behind the checker), not on the
        // checker's square. Add that target to the capture-match mask so it isn't filtered.
        uint64_t pawn_checker = my_const::pawn_neighbors[pit][ev_square] & Board.bitboard[!pit] & Board.bitboard[Pawn];
        uint64_t ep_behind = (((pawn_checker << 8) & my_const::pit_on_off[!pit]) | ((pawn_checker >> 8) & my_const::pit_on_off[pit]))
                        & Board.bitboard[status] & (my_const::row_3_mask | my_const::row_6_mask);
        for (int i = 0; i < Board.possible_moves.size(); ++i) { // king in single check - match already generated moves that blocks checker
            if (move_to_bits & (1UL << ((Board.possible_moves[i] & to_square_mask) >> 4))) temp_possible_moves.push_back(Board.possible_moves[i]);
        }
        for (int i = 0; i < Board.possible_captures.size(); ++i) { // king in single check - match already generated captures checker
            if ((move_to_bits | ep_behind) & (1UL << ((Board.possible_captures[i] & to_square_mask) >> 4))) temp_possible_captures.push_back(Board.possible_captures[i]);
        }

        Board.possible_moves = temp_possible_moves;
        Board.possible_captures = temp_possible_captures;
        move_to_bits = my_const::king_neighbors[ev_square] & (~all_pop | Board.bitboard[!pit]) & ~under_attack_bb & ~move_to_bits;
        Move.capture (ev_bit, Board.bitboard[!pit] & move_to_bits, 4, Board.possible_captures);
        Move.from_to (ev_bit, ~all_pop & move_to_bits, 0, Board.possible_moves);
        king_in_check = (Board.possible_moves.empty() & Board.possible_captures.empty());
        return king_in_check;
    }
}