#include "global_header.h"
#include "support_files.h"

#ifndef TABLES_H
#define TABLES_H

two_vectors diag_mask_lookup(void);
two_vectors rank_file_mask_lookup(void);
std::array<std::array<uint64_t, 64>, 2> build_passed_pawn_mask(void);

class Initiation {
    public:
    // initiate the lookup tables needed for PEXT
    // const two_vectors diag = diag_mask_lookup();
    // const two_vectors rank_file = rank_file_mask_lookup();
    const flat_slider<5248>   diag      = flatten<5248>   (diag_mask_lookup());
    const flat_slider<102400> rank_file = flatten<102400> (rank_file_mask_lookup());

    const std::array<std::array<uint64_t, 64>, 2> passed_pawn = build_passed_pawn_mask(); 
};
extern Initiation Lookup;      // name-match, no initialiser


class Support { 
    public:
    void from_to (uint64_t from, uint64_t moves, uint flags, MoveList &move_list) {
        uint64_t to;
        uint16_t single_move;
        
        while (moves) {
            to = moves & (~moves + 1UL); // take the rightmost bit in "moves" and make it a single "to"
            moves &= moves - 1UL; // remove the single "to" from "moves" 
            single_move = __builtin_ctzll(from) << 10 | __builtin_ctzll(to) << 4 | flags;
            move_list.push_back(single_move);
        }

        return;
    }

    void capture (uint64_t from, uint64_t moves, uint flags, MoveList &capture_list) {
        uint64_t to;
        uint16_t single_move;
        
        while (moves) {
            to = moves & (~moves + 1UL); // take the rightmost bit in "moves" and make it a single "to"
            moves &= moves - 1UL; // remove the single "to" from "moves" 
            single_move = __builtin_ctzl(from) << 10 | __builtin_ctzl(to) << 4 | flags;
            capture_list.push_back(single_move);
        }

        return;
    }

    void from_to_promotion (uint64_t from, uint64_t to, MoveList &move_list) {
        uint16_t single_move;

        for (uint i = 8; i < 12; ++i) { // generate 4 moves with flag 8, 9, 10, 11
            single_move = __builtin_ctzl(from) << 10 | __builtin_ctzl(to) << 4 | i;
            move_list.push_back(single_move);
        }

        return;
    }

    void capture_promotion (uint64_t from, uint64_t moves, MoveList &capture_list) {
        uint64_t to;
        uint16_t single_move;

        while (moves) {
            to = moves & (~moves + 1UL); // take the rightmost bit in "moves" and make it a single "to"
            moves &= moves - 1UL; // remove the single "to" from "moves" 
            for (uint j = 12; j < 16; ++j) { // generate 4 moves with flag 12, 13, 14, 15
                single_move = __builtin_ctzl(from) << 10 | __builtin_ctzl(to) << 4 | j;
                capture_list.push_back(single_move);
            }
        }
        
        return;
    }
};
extern Support Move; // name-match, no initialiser

#endif