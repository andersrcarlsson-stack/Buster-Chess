// movepicker.h
#include "support_files.h"          // uint16_t + move layout
#ifndef MOVEPICKER_H
#define MOVEPICKER_H

enum class MoveStage { Hash, GoodCapture, LosingCapture, Killer, Quiet };

class MovePicker {
public:
    MovePicker(bool has_hash, int lose_start, int quiet_start)
        : has_hash_(has_hash), lose_start_(lose_start), quiet_start_(quiet_start) {}
        MoveStage stage(int ordinal, uint16_t move) const {   // in-class = inlinable (LTO is off)
        if (has_hash_ && ordinal == 0) return MoveStage::Hash;    // the rotated hash move
        if (move & 4)                                             // capture (bit 2)
            return ordinal >= lose_start_ ? MoveStage::LosingCapture
                                          : MoveStage::GoodCapture;
        return ordinal >= quiet_start_ ? MoveStage::Quiet         // post-killer history quiets
                                       : MoveStage::Killer;        // killer region
    }
    void snapshot_quiets(const MoveList & moves, int insert, bool pit, const Chess & Board, int pp, int pt);
    void pick_next_quiet(MoveList & moves, int from);  // selection over the snapshot
private:
    bool has_hash_;
    int  lose_start_;
    int  quiet_start_;
    // NOT value-initialised: the {} compiled to a 1 KiB memset@plt on EVERY node (and every
    // q-node, where it is never even read). Safe because every read is covered by a prior
    // write: snapshot_quiets fills [insert, size), and pick_next_quiet is only ever called
    // with from >= insert, reading only [from, size).
    std::array<int, 256> qscore_;   // per-node history snapshot of the quiet region (no drift)
};

#endif