/* movepicker: a per-node phase oracle that packages the ordering boundaries
(good_caps / lose_start / insert) into a named MoveStage {Hash, GoodCapture,
LosingCapture, Killer, Quiet}, which the pruning rules in alpha_beta and q_search read,
plus the lazy selection of quiet moves by history + continuation history. */

#include "global_header.h"
#include "support_files.h"
#include "global_constants.h"
#include "movepicker.h"
#include "search_state.h"

void MovePicker::snapshot_quiets(const MoveList & moves, int insert, bool pit, const Chess & Board, int pp, int pt) {
    for (int k = insert; k < moves.size(); ++k) {
        uint16_t m = moves[k];
        int cp = Board.mailbox[_pext_u32(m, from_square_mask)];
        int ct = _pext_u32(m, to_square_mask);
        qscore_[k] = History.get(pit, m) + Continuation.get(pit, pp, pt, cp, ct);   // global + contextual
    }
}

void MovePicker::pick_next_quiet(MoveList & moves, int from) {
    int best = from;
    for (int j = from + 1; j < moves.size(); ++j)
        if (qscore_[j] > qscore_[best]) best = j;   // strict > → stable on ties
    if (best != from) {
        std::iter_swap(moves.begin() + from, moves.begin() + best);
        std::swap(qscore_[from], qscore_[best]);    // keep snapshot aligned with moves
    }
}


