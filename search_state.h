#include "global_header.h" // brings <atomic>; keeps the header self-contained

#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

class Stop_search {
public:
    std::atomic<bool> stop {false}; // default member initialisers STAY here —
    long long nodes {0}; // they're part of the type, not an object def
    long long node_limit {0};
};
extern Stop_search Stop; // DECLARATION only — no initialiser

// Tunable search parameters — exposed via UCI setoption for external tuning (e.g. SPSA).
// Defaults = the current hardcoded literals, so behaviour is IDENTICAL until an option is set.
struct Tuning {
    int futility_margin = 106;  // tuned (SPSA)
    int futility_depth  = 5;  // tuned (SPSA)
    int rfp_depth  = 8;      // tuned (SPSA)
    int rfp_margin = 70;     // tuned (SPSA)
    int delta_margin = 190;  // tuned (SPSA)
    int lmr_reduction = 2;   // unused: reductions come from the lmr table below; kept so the LmrReduction option still parses
    int lmp_depth = 3;   // late-move pruning fires only at remaining depth <= this
    int null_move_r     = 2;  // null-move search reduction R
    int check_ext       = 1;  // check extension: +this many plies
    int asp_delta = 16;      // tuned (SPSA)
    int asp_min_depth = 3;   // tuned (SPSA)

    int lmr[64][64]; // [depth][movenumber] reduction table
    Tuning() { // fill once at construction
        for (int d = 0; d < 64; ++d)
            for (int m = 0; m < 64; ++m)
                lmr[d][m] = (d >= 1 && m >= 1)
                    ? (int)(0.75 + std::log((double)d) * std::log((double)m) / 2.25)
                    : 0;
    }
    int lmr_r(int depth, int movenum) const {
        return lmr[std::min(depth, 63)][std::min(movenum, 63)];
    }
};
extern Tuning Tune; // DECLARATION only — no initialiser

// Killer moves: up to two quiet cutoff-movers per ply, indexed by ply from root.
// Kept across the iterative-deepening iterations of one search; cleared at ucinewgame.
class Killer_table {
public:
    static constexpr int max_ply {64};   // plies deeper than this simply get no killers
    static constexpr int slots   {2};    // two entries per ply, FIFO push-out

    // store a QUIET cutoff move: newest goes to slot 0, the old slot 0 shifts down to slot 1
    void store (int ply, uint16_t move) {
        if (ply < 0 or ply >= max_ply) return;   // out of range - ignore silently
        if (table[ply][0] == move) return;       // already the primary killer - don't duplicate
        table[ply][1] = table[ply][0];           // FIFO: old primary becomes secondary
        table[ply][0] = move;
    }
    // read one slot; 0 means "no killer" (same sentinel as hash_move)
    uint16_t get (int ply, int slot) const {
        if (ply < 0 or ply >= max_ply) return 0;
        return table[ply][slot];
    }

    void clear () { table = {}; }   // zero every slot - call from ucinewgame

private:
    std::array<std::array<uint16_t, slots>, max_ply> table {};

};
extern Killer_table Killers; // DECLARATION only — no initialiser

// The move played to reach each ply, as (moving piece, to-square) — the context key
// for continuation history. Ply-indexed from root; a null
// move records {0,0}. Always overwritten on the way down, so it needs no clearing.
class Move_stack {
public:
    static constexpr int max_ply {64};
    void set (int ply, int piece, int to) {
        if (ply < 0 or ply >= max_ply) return;
        pc[ply] = piece; sq[ply] = to;
    }
    int piece (int ply) const { return (ply >= 0 and ply < max_ply) ? pc[ply] : 0; }
    int to    (int ply) const { return (ply >= 0 and ply < max_ply) ? sq[ply] : 0; }
private:
    std::array<int, max_ply> pc {};
    std::array<int, max_ply> sq {};
};
extern Move_stack Line;   // DECLARATION only

// History heuristic: global cutoff track record for quiet moves, indexed [side][from][to].
// Orders the quiet tail below the killers. Cleared at ucinewgame.
class History_table {
public:
    static constexpr int max_score {16384};   // must fit int16_t; power of 2 -> the divide is a shift

    // Beta-cutoff: the move that caused it. QUIET moves only. depth = remaining depth.
    void update (bool pit, uint16_t move, int depth) {
        apply(pit, move, std::min(depth * depth, max_score));
    }

    // The quiets that were tried BEFORE the cutoff move and failed. Same magnitude, opposite sign.
    void penalise (bool pit, uint16_t move, int depth) {
        apply(pit, move, -std::min(depth * depth, max_score));
    }

    int get (bool pit, uint16_t move) const {
        return table[pit][_pext_u32(move, from_square_mask)][_pext_u32(move, to_square_mask)];
    }

    void clear () { table = {}; }

private:
    void apply (bool pit, uint16_t move, int bonus) {
        int16_t & entry = table[pit][_pext_u32(move, from_square_mask)][_pext_u32(move, to_square_mask)];
        // History gravity: entry*(1-t) + bonus with t = |bonus|/max_score, so entry can never leave
        // [-max_score, max_score] - now in BOTH directions. Do NOT bracket as
        // entry * (abs(bonus)/max_score) - the integer divide rounds to 0 and the bound vanishes.
        // int16_t promotes to int, so the ~2.7e8 product is fine.
        entry += bonus - entry * std::abs(bonus) / max_score;
    }

    std::array<std::array<std::array<int16_t, 64>, 64>, 2> table {};   // 16 KB
};
extern History_table History; // DECLARATION only — no initialiser

// Continuation history: History (butterfly) CONDITIONED on the previous move. Scores a quiet
// by how it has performed when played right after (prev_piece -> prev_to) — so "knight to e5
// after ...d5" learns independently of the global average. Indexed
// [side][prev_piece][prev_to][cur_piece][cur_to]; same bounded gravity as History.
class Continuation_table {
public:
    static constexpr int max_score {16384};
    void update   (bool pit, int pp, int pt, int cp, int ct, int depth) { apply(pit,pp,pt,cp,ct,  std::min(depth*depth, max_score)); }
    void penalise (bool pit, int pp, int pt, int cp, int ct, int depth) { apply(pit,pp,pt,cp,ct, -std::min(depth*depth, max_score)); }
    int  get      (bool pit, int pp, int pt, int cp, int ct) const { return table[pit][pp][pt][cp][ct]; }
    void clear () { table = {}; }
private:
    void apply (bool pit, int pp, int pt, int cp, int ct, int bonus) {
        int16_t & e = table[pit][pp][pt][cp][ct];
        e += bonus - e * std::abs(bonus) / max_score;   // identical gravity to History_table
    }
    std::array<std::array<std::array<std::array<std::array<int16_t, 64>, 8>, 64>, 8>, 2> table {}; // 2*8*64*8*64*2 = 1 MB
};
extern Continuation_table Continuation;   // DECLARATION only

class Static_Exchange_Evaluation {
public:
    int SEE_Sort(Chess & Board);
    int see_value(Chess & Board, uint16_t move);   // swap-off for ONE move
    u_int64_t attackers_to(Chess & board, int X, uint64_t occ);
};
extern Static_Exchange_Evaluation SEE; // DECLARATION only — no initialiser

#endif