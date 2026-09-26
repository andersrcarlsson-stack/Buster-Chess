#include "global_header.h"
#include "support_files.h"
#include "global_constants.h"

std::vector<uint64_t> binary_permut(uint64_t);
uint64_t generate_diag_moves(int, uint64_t, uint64_t);
uint64_t generate_rank_file_moves(int, uint64_t, uint64_t);
int leftmostSetBit_0(uint64_t n);

std::array<std::array<uint64_t, 64>, 2> build_passed_pawn_mask(void) {
    std::array<std::array<uint64_t, 64>, 2> mask {};
    for (int sq = 0; sq < 64; ++sq) {
        int f = sq & 7, r = sq >> 3;                              // file 0..7, rank 0..7
        uint64_t files = 0x0101010101010101ULL << f;              // own file
        if (f > 0) files |= 0x0101010101010101ULL << (f - 1);     // left-adjacent file
        if (f < 7) files |= 0x0101010101010101ULL << (f + 1);     // right-adjacent file
        mask[0][sq] = files & ((r < 7) ? (~0ULL << ((r + 1) * 8)) : 0ULL);  // White: ranks AHEAD (up)
        mask[1][sq] = files & ((r > 0) ? ((1ULL << (r * 8)) - 1) : 0ULL);   // Black: ranks AHEAD (down)
    }
    return mask;
}


two_vectors diag_mask_lookup(void)   {
    uint64_t permutation;
    std::vector <uint64_t> diag_blockers;
    std::vector <uint64_t> permutation_of_blockers;
    std::vector<std::vector<uint64_t>> diag_moves;
    std::vector<uint64_t> square_moves;
    int square;
    two_vectors test;
    for (int y = 1; y < 9; ++y)    {
        for (int x = 1; x < 9; ++x)    {
            square = (x - 1) + ((y - 1) * 8);
            int y_offset = 1;
            uint64_t board_part = 0ULL;
            uint64_t board = 0ULL;
            for (int a = x + 1; a < 8; ++a)    {
                if (y + y_offset < 8) { 
                    board_part = 1ULL << ((a - 1)  + (y + y_offset - 1) * 8);
                    board += board_part;
                }
                if (y - y_offset > 1) {

                    board_part = 1ULL << ((a - 1)  + (y - y_offset - 1) * 8);
                    board += board_part;
                }
                ++y_offset;
            }
            y_offset = 1;
            for (int b = x - 1; b > 1; --b)   {
                if (y + y_offset < 8) {
                    board_part = 1ULL << ((b - 1)  + (y + y_offset - 1) * 8 );
                    board += board_part;
                }
                if (y - y_offset > 1) {

                    board_part = 1ULL << ((b - 1) + (y - y_offset - 1) * 8);
                    board += board_part;
                }
                ++y_offset;
            }
            permutation_of_blockers = binary_permut(board);
            diag_blockers.push_back(board);
            for (uint64_t zzz : permutation_of_blockers) {
                permutation = generate_diag_moves(square, 1ULL << square, zzz);
                square_moves.push_back(permutation);
            }
            diag_moves.push_back(square_moves);
            square_moves.clear();
        }
    }

    test.mask = diag_blockers;
    test.lookup = diag_moves;

    return test;
}

uint64_t generate_diag_moves(int square, uint64_t evaluated_bit, uint64_t blockers) {
uint64_t low_population, high_population, low_horizon_mask, high_horizon_mask, low_opponent, high_opponent;
// generates moves for Diagonal Sliders 
    uint64_t possible_move_to_bits = 0ULL;
    uint64_t possible_capture_bits = 0ULL;
    uint64_t low_mask = evaluated_bit - 1ULL; // set everything lower than "evaluated_bit" to 1
    uint64_t high_mask = evaluated_bit ^ (~low_mask); // set everything higher than "evaluated_bit" to 1
    for (int m : my_const::diagonal_pointer[square]) { // loop for one or two diagonals based on the particular square number (int)
        low_population = blockers & my_const::diagonals[m] & low_mask; // look in lower diagonal
        high_population = blockers & my_const::diagonals[m] & high_mask; // look in higher diagonal
        low_horizon_mask = ~((1ULL << (leftmostSetBit_0(low_population))) - 1ULL); // mask out everything lower than leftmost populated pos
        high_horizon_mask = (high_population & (~high_population + 1ULL)) - 1ULL; // mask out everything higher than rightmost populated pos
        low_opponent = blockers & ((1ULL << leftmostSetBit_0(low_population)) >> 1); // look in lower diagonal
        high_opponent = blockers & (high_population & (~high_population +1ULL)); // look in higher diagonal

        possible_move_to_bits |= (high_horizon_mask & low_horizon_mask & my_const::diagonals[m] & ~evaluated_bit); // combine the high and low horizon masks to get possible move space - Note Queen itself
        possible_capture_bits |= (low_opponent | high_opponent);

        possible_move_to_bits |= possible_capture_bits;
    
    }

return possible_move_to_bits;
}

two_vectors rank_file_mask_lookup(void)   {
    uint64_t permutation;
    std::vector <uint64_t> rank_file_blockers;
    std::vector <uint64_t> permutation_of_blockers;
    std::vector<std::vector<uint64_t>> rank_file_moves;
    std::vector<uint64_t> square_moves;
    int square;
    two_vectors test;
    for (int y = 1; y < 9; ++y)   {
        for (int x = 1; x < 9; ++x)  {
            square = (x - 1) + ((y - 1) * 8);
            uint64_t board_part = 0ULL;
            uint64_t board = 0ULL;
            for (int a = 1; a < 8; ++a)  {
                if (x + a < 8) { 
                    board_part = 1ULL << (square + a);
                    board += board_part;
                }
                if (x - a > 1) {
                    board_part = 1ULL << (square - a);
                    board += board_part;
                }
            }
            for (int b = 1; b < 8; ++b)    {
                if (y + b < 8) {
                    board_part = 1ULL << (square + b*8);
                    board += board_part;
                }
                if (y - b > 1) {
                    board_part = 1ULL << (square - b*8);
                    board += board_part;
                }
            }
            permutation_of_blockers = binary_permut(board);
            rank_file_blockers.push_back(board);
            for (uint64_t zzz : permutation_of_blockers) {
                permutation = generate_rank_file_moves(square, 1ULL << square, zzz);
                square_moves.push_back(permutation);
            }
            rank_file_moves.push_back(square_moves);
            square_moves.clear();
        }
    }

    test.mask = rank_file_blockers;
    test.lookup = rank_file_moves;

    return test;
}

uint64_t generate_rank_file_moves(int square, uint64_t evaluated_bit, uint64_t blockers) {
uint64_t low_population, high_population, low_horizon_mask, high_horizon_mask, low_opponent, high_opponent;
// generates moves for Rank and File Sliders 
    uint64_t possible_move_to_bits = 0ULL;
    uint64_t possible_capture_bits = 0ULL;
    uint64_t low_mask = evaluated_bit - 1ULL; // set everything lower than "evaluated_bit" to 1
    uint64_t high_mask = evaluated_bit ^ (~low_mask); // set everything higher than "evaluated_bit" to 1
    for (int m : my_const::ranks_and_files_pointer[square]) { // loop for one or two diagonals based on the particular square number (int)
        low_population = blockers & my_const::ranks_and_files[m] & low_mask; // look in lower diagonal
        high_population = blockers & my_const::ranks_and_files[m] & high_mask; // look in higher diagonal
        low_horizon_mask = ~((1ULL << (leftmostSetBit_0(low_population))) - 1ULL); // mask out everything lower than leftmost populated pos
        high_horizon_mask = (high_population & (~high_population + 1ULL)) - 1ULL; // mask out everything higher than rightmost populated pos
        low_opponent = blockers & ((1ULL << leftmostSetBit_0(low_population)) >> 1); // look in lower diagonal
        high_opponent = blockers & (high_population & (~high_population +1ULL)); // look in higher diagonal

        possible_move_to_bits |= (high_horizon_mask & low_horizon_mask & my_const::ranks_and_files[m] & ~evaluated_bit); // combine the high and low horizon masks to get possible move space - Note Queen itself
        possible_capture_bits |= (low_opponent | high_opponent);
        possible_move_to_bits |= possible_capture_bits;
    
    }

return possible_move_to_bits;
}

int leftmostSetBit_0(uint64_t n) {
        
        return (!n)? 0 : 64 - std::countl_zero(n);
    };

void generatePermutations(uint64_t num, std::vector<uint64_t>& results, int index = 0) {
    if (index == 64) {
        results.push_back(num);
        return;
    }
    generatePermutations(num, results, index + 1);
    
    if (num & (1ULL << index)) {
        num ^= (1ULL << index);
        generatePermutations(num, results, index + 1);
    }
}

std::vector<uint64_t> getBinaryPermutations(uint64_t input) {
    std::vector<uint64_t> results;
    generatePermutations(input, results);
    std::sort(results.begin(), results.end());
    return results;
}

std::vector<uint64_t> binary_permut (uint64_t input) {
    std::vector<uint64_t> permut;
    permut = {};
    std::vector<uint64_t> permutations = getBinaryPermutations(input);
    
    for (const auto& perm : permutations) {
        permut.push_back((perm));
    }
    
    return permut;
}
