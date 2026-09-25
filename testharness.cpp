#include "global_header.h"
#include "support_files.h"    // Chess, MoveList, + decls: generate_moves/make/unmake/make_new_state/evaluation/perft
#include "global_constants.h" // my_const (mirror_board)
#include "piece_table.h"
#include "tt.h"               // zobrist (verify_walk's key checks)
#include "testharness.h"
#include "movegen.h"
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null
#include "eval.h"

// Vertically mirror the board, swap colours, and swap side-to-move — the "null" transform
// that a correct, side-to-move-relative eval must be INVARIANT under: eval(b) == eval(mirror(b)).
// It's how we police the piece-square tables: any asymmetric entry (a mistyped mirror, a dropped
// comma) makes the two evals disagree — a bug that is otherwise invisible to perft, to the key
// checks, and to self-play (both sides share the broken table). Only the fields evaluation() reads
// are transformed (the 8 piece/occupancy bitboards + pit); status/key/mailbox are irrelevant to it.
//   bswap64 flips ranks 1<->8 (each rank is one byte); swapping [White]/[Black] swaps the colours.
Chess mirror_board(const Chess & b) {
    Chess m = b;
    for (int i = 0; i < 8; ++i) m.bitboard[i] = __builtin_bswap64(b.bitboard[i]);
    uint64_t tmp = m.bitboard[White]; m.bitboard[White] = m.bitboard[Black]; m.bitboard[Black] = tmp;
    m.pit = !b.pit;
    return m;
}

void verify_walk(const Chess & Board, int depth, TestHarness & tests) {
    if (depth == 0) return;
    for (uint16_t move : Board.possible_moves) {
        Chess child = Board;
        make_new_state(child, move);
        generate_moves(child);
        uint64_t incremental = child.tpt.zobrist_key_64;
        Chess recompute = child;
        zobrist.generate_initial_zobrist_key(recompute);
        tests.check(recompute.tpt.zobrist_key_64 == incremental, "zobrist key consistency");

        // make/unmake round-trip — make then unmake must restore the board's STATE.
        Chess rt = Board;                       // copy of the parent (state + move list)
        Undo undo;
        make(rt, move, undo);                   // rt becomes the child
        unmake(rt, move, undo);                 // ... and back
        tests.check(rt.bitboard == Board.bitboard and rt.mailbox == Board.mailbox
                    and rt.pit == Board.pit and rt.tpt.zobrist_key_64 == Board.tpt.zobrist_key_64,
                    "make/unmake round-trip");

        // eval symmetry: the mirrored position must evaluate identically (see mirror_board).
        tests.check(evaluation(child) == evaluation(mirror_board(child)), "eval symmetry");

        // null-move primitive: after a pass the incremental key must match a from-scratch recompute
        // (e.p. cleared, stm flipped), and make_null/unmake_null must restore the board exactly.
        Chess nb = child;
        Undo nu;
        make_null(nb, nu);
        Chess nb_rc = nb;
        zobrist.generate_initial_zobrist_key(nb_rc);
        tests.check(nb_rc.tpt.zobrist_key_64 == nb.tpt.zobrist_key_64, "null-move key consistency");
        unmake_null(nb, nu);
        tests.check(nb.bitboard == child.bitboard and nb.mailbox == child.mailbox
                    and nb.pit == child.pit and nb.tpt.zobrist_key_64 == child.tpt.zobrist_key_64,
                    "null-move round-trip");

        verify_walk(child, depth - 1, tests);
    }
}

// recursive make/unmake perft — ONE board, mutated on the way down and restored on the way up.
int long long perft_mu (Chess & board, int depth) {
    if (depth == 1) return board.possible_moves.size();      // leaves one ply below this node
    int long long nodes = 0;
    MoveList moves = board.possible_moves;                     // copy: make() clears board's list (now a stack copy)
    for (uint16_t move : moves) {
        Undo undo;
        make(board, move, undo);
        generate_moves(board);                        // child's move list for the next ply
        nodes += perft_mu(board, depth - 1);
        unmake(board, move, undo);                             // restore this node exactly
    }
    return nodes;
}

// public perft: same node-count semantics as the old iterative version, plus the per-root-move
// "divide" printout. Takes the board BY VALUE (owns a mutable copy) so callers are unchanged.
int long long perft (Chess Board, int depth) {
    if (depth == 0) return 1;
    int long long total = 0;
    MoveList moves = Board.possible_moves;
    for (uint16_t move : moves) {
        Undo undo;
        make(Board, move, undo);
        generate_moves(Board);
        int long long n = (depth == 1) ? 1 : perft_mu(Board, depth - 1);
        unmake(Board, move, undo);
        total += n;
        int from = (move & from_square_mask) >> 10, to = (move & to_square_mask) >> 4;
        std::cout << my_const::square_name[from] << my_const::square_name[to] << "  " << n << " \n";  // divide
    }
    return total;
}