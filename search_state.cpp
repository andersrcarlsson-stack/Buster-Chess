#include "global_header.h"
#include "support_files.h"
#include "global_constants.h"
#include "piece_table.h"
#include "search_state.h"
#include "tables.h" // SEE::attackers_to uses Lookup

Stop_search Stop; // THE single definition (members get {false}/{0} from the class)
Tuning Tune; // THE single definition (members get the hardcoded defaults from the class)
Killer_table Killers; // THE single definition 
History_table History; // THE single definition
Continuation_table Continuation; // THE single definition
Move_stack Line; // THE single definition
Static_Exchange_Evaluation SEE; // THE single definition

// Plain aggregate on purpose: std::pair has a user-provided default ctor that VALUE-initialises,
// so std::array<std::pair<int,uint16_t>,256> would emit a 2 KiB memset per node -- reintroducing
// a per-node memset for nothing. An aggregate with no user-provided ctor is default-initialised,
// i.e. no code at all.
struct SeeEntry { int score; uint16_t move; };

// Static exchange value of ONE move, from the mover's point of view.
// Positive = the move wins material. Extracted verbatim from SEE_Sort's inner loop,
// so capture ordering is identical to the sort.
//
// It works for a QUIET move with no change at all: mailbox[] holds 0 on an empty
// square and piece_value[0] == 0, so gain[0] falls out as 0 and the sequence simply
// starts with the opponent trying to win the piece that just arrived.
// NOT valid for king moves (piece_value[King] = 1000 makes the swap-off nonsense)
// or promotions (the value on the square changes mid-sequence) — the caller excludes both.
// Pins are ignored, as in every SEE: a pinned defender still "recaptures" here.
int Static_Exchange_Evaluation::see_value(Chess & Board, uint16_t move) {
    uint64_t occ  = Board.bitboard[0] | Board.bitboard[1];
    int ev_square = _pext_u32(move, to_square_mask);
    int from_sq   = _pext_u32(move, from_square_mask);
    int gain[32];
    gain[0] = my_const::piece_value[Board.mailbox[ev_square]];   // 0 when the square is empty
    int onsquare = Board.mailbox[from_sq];
    int side = 1 ^ Board.pit;
    occ &= ~(1ULL << from_sq);
    uint64_t attackers = my_const::knight_neighbors[ev_square] & Board.bitboard[Knight];
    attackers |= my_const::king_neighbors[ev_square] & Board.bitboard[King];
    attackers |= my_const::pawn_neighbors[Black][ev_square] & Board.bitboard[Pawn] & Board.bitboard[White];
    attackers |= my_const::pawn_neighbors[White][ev_square] & Board.bitboard[Pawn] & Board.bitboard[Black];
    attackers |= attackers_to(Board, ev_square, occ);

    int d = 0, piece;
    uint64_t lva {}, ev_bit {};
    while (true) {
        lva = attackers & occ & Board.bitboard[side];
        if (lva == 0) break;
        for (int j = 2; j < 8; ++j) {
            if (lva & Board.bitboard[j]) {
                ev_bit = (lva & Board.bitboard[j]) & (~(lva & Board.bitboard[j]) + 1ULL);
                break;
            }
        }
        piece = Board.mailbox[std::countr_zero(ev_bit)];
        ++d;
        gain[d] = my_const::piece_value[onsquare] - gain[d-1];
        onsquare = piece;
        occ &= ~ev_bit;
        attackers |= attackers_to(Board, ev_square, occ);   // |= keeps knight/king/pawn attackers and adds revealed x-rays
        side ^= 1;
    }
    while (d > 0) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
        --d;
    }
    return gain[0];
}

int Static_Exchange_Evaluation::SEE_Sort(Chess & Board) {
    std::array<SeeEntry, 256> sort_list;  // fixed buffer: no per-node heap alloc
    int n = 0;
    MoveList sorted_captures;
    for (int i = 0; i < Board.possible_captures.size(); ++i) {
        sort_list[n++] = { -see_value(Board, Board.possible_captures[i]), Board.possible_captures[i] };
    }

    std::sort(sort_list.begin(), sort_list.begin() + n,
              [](const SeeEntry & a, const SeeEntry & b) {    // == std::pair's operator<
                  return a.score < b.score or (a.score == b.score and a.move < b.move);
              });
    int good_captures = 0;
    for (int k = 0; k < n; ++k) if (sort_list[k].score <= 0) ++good_captures;
    for (int k = 0; k < n; ++k) sorted_captures.push_back(sort_list[k].move);

    Board.possible_captures = sorted_captures;
    MoveList ordered = sorted_captures;
    for (uint16_t m : Board.possible_moves)
        if (!(m & 4)) ordered.push_back(m);   // quiets only — bit 2 is the capture flag
    Board.possible_moves = ordered;

    return good_captures;
};

// for SEE calculation - take the battle exchange square X, and the board occupancy occ and querry available lot:s for attackers 
uint64_t Static_Exchange_Evaluation::attackers_to(Chess & board, int X, uint64_t occ) {
    uint64_t attackers = Lookup.diag.attacks(X, occ) & (board.bitboard[Bishop] | board.bitboard[Queen]);
    attackers |= Lookup.rank_file.attacks(X, occ) & (board.bitboard[Rook] | board.bitboard[Queen]); 
    
    return attackers;
};
