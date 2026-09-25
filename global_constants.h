#include "global_header.h"

#ifndef GLOBAL_CONSTANTS_H
#define GLOBAL_CONSTANTS_H

constexpr uint64_t 
a1 = 1, b1 = 2, c1 = 4, d1 = 8, e1 = 16, f1 = 32, g1 = 64, h1 = 128,
a2 = 256, b2 = 512, c2 = 1024, d2 = 2048, e2 = 4096, f2 = 8192, g2 = 16384, h2 = 32768,
a3 = 65536, b3 = 131072, c3 = 262144, d3 = 524288, e3 = 1048576, f3 = 2097152, g3 = 4194304, h3 = 8388608,
a4 = 16777216, b4 = 33554432, c4 = 67108864, d4 = 134217728, e4 = 268435456, f4 = 536870912, g4 = 1073741824, h4 = 2147483648,
a5 = 4294967296, b5 = 8589934592, c5 = 17179869184, d5 = 34359738368, e5 = 68719476736, f5 = 137438953472, g5 = 274877906944, h5 = 549755813888,
a6 = 1099511627776, b6 = 2199023255552, c6 = 4398046511104, d6 = 8796093022208, e6 = 17592186044416, f6 = 35184372088832, g6 = 70368744177664, h6 = 140737488355328,
a7 = 281474976710656, b7 = 562949953421312, c7 = 1125899906842624, d7 = 2251799813685248, e7 = 4503599627370496, f7 = 9007199254740992, g7 = 18014398509481984, h7 = 36028797018963968,
a8 = 72057594037927936, b8 = 144115188075855872, c8 = 288230376151711744, d8 = 576460752303423488, e8 = 1152921504606846976, f8 = 2305843009213693952, g8 = 4611686018427387904, h8 = 9223372036854775808UL;

namespace my_const {

constexpr std::array<uint64_t, 28> diagonals {
        a1+b2+c3+d4+e5+f6+g7+h8, a2+b3+c4+d5+e6+f7+g8, a3+b4+c5+d6+e7+f8, a4+b5+c6+d7+e8, a5+b6+c7+d8, a6+b7+c8, a7+b8, b1+c2+d3+e4+f5+g6+h7, c1+d2+e3+f4+g5+h6, d1+e2+f3+g4+h5,
        e1+f2+g3+h4, f1+g2+h3, g1+h2, a8+b7+c6+d5+e4+f3+g2+h1, b8+c7+d6+e5+f4+g3+h2, c8+d7+e6+f5+g4+h3, d8+e7+f6+g5+h4,  e8+f7+g6+h5, f8+g7+h6, g8+h7, a7+b6+c5+d4+e3+f2+g1, 
        a6+b5+c4+d3+e2+f1, a5+b4+c3+d2+e1, a4+b3+c2+d1, a3+b2+c1, a2+b1, ~0UL, 0UL}; 
        // element #27 (index 26) is for all board OK (pinned pieces)

constexpr std::array<std::array<uint64_t, 2>, 65> diagonal_pointer {{
        {0, 0}, {7, 25}, {8, 24}, {9, 23}, {10, 22}, {11, 21}, {12, 20}, {13, 13}, {1, 25}, {0, 24}, {7, 23}, {8, 22}, {9, 21}, {10, 20}, {11, 13}, {12, 14}, 
        {2, 24}, {1, 23}, {0, 22}, {7, 21}, {8, 20}, {9, 13}, {10, 14}, {11, 15}, {3, 23}, {2, 22}, {1, 21}, {0, 20}, {7, 13}, {8, 14}, {9, 15}, {10, 16}, 
        {4, 22}, {3, 21}, {2, 20}, {1, 13}, {0, 14}, {7, 15}, {8, 16}, {9, 17}, {5, 21}, {4, 20}, {3, 13}, {2, 14}, {1, 15}, {0, 16}, {7, 17}, {8, 18},
        {6, 20}, {5, 13}, {4, 14}, {3, 15}, {2, 16}, {1, 17}, {0, 18}, {7, 19}, {13, 13}, {6, 14}, {5, 15}, {4, 16}, {3, 17}, {2, 18}, {1, 19}, {0, 0}, {26, 27}}}; 

constexpr std::array<uint64_t, 18> ranks_and_files {
        a1+b1+c1+d1+e1+f1+g1+h1, a2+b2+c2+d2+e2+f2+g2+h2, a3+b3+c3+d3+e3+f3+g3+h3, a4+b4+c4+d4+e4+f4+g4+h4, a5+b5+c5+d5+e5+f5+g5+h5, a6+b6+c6+d6+e6+f6+g6+h6, a7+b7+c7+d7+e7+f7+g7+h7, a8+b8+c8+d8+e8+f8+g8+h8,
        a1+a2+a3+a4+a5+a6+a7+a8, b1+b2+b3+b4+b5+b6+b7+b8, c1+c2+c3+c4+c5+c6+c7+c8, d1+d2+d3+d4+d5+d6+d7+d8, e1+e2+e3+e4+e5+e6+e7+e8, f1+f2+f3+f4+f5+f6+f7+f8, g1+g2+g3+g4+g5+g6+g7+g8, h1+h2+h3+h4+h5+h6+h7+h8, ~0UL, 0UL};
        // element # 17 (index 16) is for all board OK (pinned pieces)

constexpr std::array<std::array<uint64_t, 2>, 65> ranks_and_files_pointer {{
        {0, 8}, {0, 9}, {0, 10}, {0, 11}, {0, 12}, {0, 13}, {0, 14}, {0, 15}, {1, 8}, {1, 9}, {1, 10}, {1, 11}, {1, 12}, {1, 13}, {1, 14}, {1, 15}, 
        {2, 8}, {2, 9}, {2, 10}, {2, 11}, {2, 12}, {2, 13}, {2, 14}, {2, 15}, {3, 8}, {3, 9}, {3, 10}, {3, 11}, {3, 12}, {3, 13}, {3, 14}, {3, 15}, 
        {4, 8}, {4, 9}, {4, 10}, {4, 11}, {4, 12}, {4, 13}, {4, 14}, {4, 15}, {5, 8}, {5, 9}, {5, 10}, {5, 11}, {5, 12}, {5, 13}, {5, 14}, {5, 15},
        {6, 8}, {6, 9}, {6, 10}, {6, 11}, {6, 12}, {6, 13}, {6, 14}, {6, 15}, {7, 8}, {7, 9}, {7, 10}, {7, 11}, {7, 12}, {7, 13}, {7, 14}, {7, 15}, {16, 17}}};

constexpr std::array<uint64_t, 64> king_neighbors {
        a2+b2+b1, a1+a2+b2+c2+c1, b1+b2+c2+d2+d1, c1+c2+d2+e2+e1, d1+d2+e2+f2+f1, e1+e2+f2+g2+g1, f1+f2+g2+h2+h1, g1+g2+h2, 
        a3+b3+b2+b1+a1, a1+a2+a3+b3+c3+c2+c1+b1, b1+b2+b3+c3+d3+d2+d1+c1, c1+c2+c3+d3+e3+e2+e1+d1, d1+d2+d3+e3+f3+f2+f1+e1, e1+e2+e3+f3+g3+g2+g1+f1, f1+f2+f3+g3+h3+h2+h1+g1, g1+g2+g3+h3+h1,
        a4+b4+b3+b2+a2, a2+a3+a4+b4+c4+c3+c2+b2, b2+b3+b4+c4+d4+d3+d2+c2, c2+c3+c4+d4+e4+e3+e2+d2, d2+d3+d4+e4+f4+f3+f2+e2, e2+e3+e4+f4+g4+g3+g2+f2, f2+f3+f4+g4+h4+h3+h2+g2, g2+g3+g4+h4+h2,
        a5+b5+b4+b3+a3, a3+a4+a5+b5+c5+c4+c3+b3, b3+b4+b5+c5+d5+d4+d3+c3, c3+c4+c5+d5+e5+e4+e3+d3, d3+d4+d5+e5+f5+f4+f3+e3, e3+e4+e5+f5+g5+g4+g3+f3, f3+f4+f5+g5+h5+h4+h3+g3, g3+g4+g5+h5+h3,
        a6+b6+b5+b4+a4, a4+a5+a6+b6+c6+c5+c4+b4, b4+b5+b6+c6+d6+d5+d4+c4, c4+c5+c6+d6+e6+e5+e4+d4, d4+d5+d6+e6+f6+f5+f4+e4, e4+e5+e6+f6+g6+g5+g4+f4, f4+f5+f6+g6+h6+h5+h4+g4, g4+g5+g6+h6+h4,
        a7+b7+b6+b5+a5, a5+a6+a7+b7+c7+c6+c5+b5, b5+b6+b7+c7+d7+d6+d5+c5, c5+c6+c7+d7+e7+e6+e5+d5, d5+d6+d7+e7+f7+f6+f5+e5, e5+e6+e7+f7+g7+g6+g5+f5, f5+f6+f7+g7+h7+h6+h5+g5, g5+g6+g7+h7+h5,
        a8+b8+b7+b6+a6, a6+a7+a8+b8+c8+c7+c6+b6, b6+b7+b8+c8+d8+d7+d6+c6, c6+c7+c8+d8+e8+e7+e6+d6, d6+d7+d8+e8+f8+f7+f6+e6, e6+e7+e8+f8+g8+g7+g6+f6, f6+f7+f8+g8+h8+h7+h6+g6, g6+g7+g8+h8+h6,
        b8+b7+a7, a7+a8+c8+c7+b7, b7+b8+d8+d7+c7, c7+c8+e8+e7+d7, d7+d8+f8+f7+e7, e7+e8+g8+g7+f7, f7+f8+h8+h7+g7, g7+g8+h7};

constexpr std::array<std::array <uint64_t, 64>, 2> king_safety {{{
        a2+b2+b1+a3+b3, a1+a2+b2+c2+c1+a3+b3+c3, b1+b2+c2+d2+d1+b3+c3+d3, c1+c2+d2+e2+e1+c3+d3+e3, d1+d2+e2+f2+f1+d3+e3+f3, e1+e2+f2+g2+g1+e3+f3+g3, f1+f2+g2+h2+h1+f3+g3+h3, g1+g2+h2+g3+h3, 
        a3+b3+b2+b1+a1+a4+b4, a1+a2+a3+b3+c3+c2+c1+b1+a4+b4+c4, b1+b2+b3+c3+d3+d2+d1+c1+b4+c4+d4, c1+c2+c3+d3+e3+e2+e1+d1+c4+d4+e4, d1+d2+d3+e3+f3+f2+f1+e1+d4+e4+f4, e1+e2+e3+f3+g3+g2+g1+f1+e4+f4+g4, f1+f2+f3+g3+h3+h2+h1+g1+f4+g4+h4, g1+g2+g3+h3+h1+g4+h4,
        a4+b4+b3+b2+a2+a5+b5, a2+a3+a4+b4+c4+c3+c2+b2+a5+b5+c5, b2+b3+b4+c4+d4+d3+d2+c2+b5+c5+d5, c2+c3+c4+d4+e4+e3+e2+d2+c5+d5+e5, d2+d3+d4+e4+f4+f3+f2+e2+d5+e5+f5, e2+e3+e4+f4+g4+g3+g2+f2+e5+f5+g5, f2+f3+f4+g4+h4+h3+h2+g2+f5+g5+h5, g2+g3+g4+h4+h2+g5+h5,
        a5+b5+b4+b3+a3+a6+b6, a3+a4+a5+b5+c5+c4+c3+b3+a6+b6+c6, b3+b4+b5+c5+d5+d4+d3+c3+b6+c6+d6, c3+c4+c5+d5+e5+e4+e3+d3+c6+d6+e6, d3+d4+d5+e5+f5+f4+f3+e3+d6+e6+f6, e3+e4+e5+f5+g5+g4+g3+f3+e6+f6+g6, f3+f4+f5+g5+h5+h4+h3+g3+f6+g6+h6, g3+g4+g5+h5+h3+g6+h6,
        a6+b6+b5+b4+a4+a7+b7, a4+a5+a6+b6+c6+c5+c4+b4+a7+b7+c7, b4+b5+b6+c6+d6+d5+d4+c4+b7+c7+d7, c4+c5+c6+d6+e6+e5+e4+d4+c7+d7+e7, d4+d5+d6+e6+f6+f5+f4+e4+d7+e7+f7, e4+e5+e6+f6+g6+g5+g4+f4+e7+f7+g7, f4+f5+f6+g6+h6+h5+h4+g4+f7+g7+h7, g4+g5+g6+h6+h4+g7+h7,
        a7+b7+b6+b5+a5+a8+b8, a5+a6+a7+b7+c7+c6+c5+b5+a8+b8+c8, b5+b6+b7+c7+d7+d6+d5+c5+b8+c8+d8, c5+c6+c7+d7+e7+e6+e5+d5+c8+d8+e8, d5+d6+d7+e7+f7+f6+f5+e5+d8+e8+f8, e5+e6+e7+f7+g7+g6+g5+f5+e8+f8+g8, f5+f6+f7+g7+h7+h6+h5+g5+f8+g8+h8, g5+g6+g7+h7+h5+g8+h8,
        a8+b8+b7+b6+a6, a6+a7+a8+b8+c8+c7+c6+b6, b6+b7+b8+c8+d8+d7+d6+c6, c6+c7+c8+d8+e8+e7+e6+d6, d6+d7+d8+e8+f8+f7+f6+e6, e6+e7+e8+f8+g8+g7+g6+f6, f6+f7+f8+g8+h8+h7+h6+g6, g6+g7+g8+h8+h6,
        b8+b7+a7+a6+b6, a7+a8+c8+c7+b7+a6+b6+c6, b7+b8+d8+d7+c7+b6+c6+d6, c7+c8+e8+e7+d7+c6+d6+e6, d7+d8+f8+f7+e7+d6+e6+f6, e7+e8+g8+g7+f7+e6+f6+g6, f7+f8+h8+h7+g7+f6+g6+h6, g7+g8+h7+g6+h6},
        {
        a2+b2+b1+a3+b3, a1+a2+b2+c2+c1+a3+b3+c3, b1+b2+c2+d2+d1+b3+c3+d3, c1+c2+d2+e2+e1+c3+d3+e3, d1+d2+e2+f2+f1+d3+e3+f3, e1+e2+f2+g2+g1+e3+f3+g3, f1+f2+g2+h2+h1+f3+g3+h3, g1+g2+h2+g3+h3, 
        a3+b3+b2+b1+a1, a1+a2+a3+b3+c3+c2+c1+b1, b1+b2+b3+c3+d3+d2+d1+c1, c1+c2+c3+d3+e3+e2+e1+d1, d1+d2+d3+e3+f3+f2+f1+e1, e1+e2+e3+f3+g3+g2+g1+f1, f1+f2+f3+g3+h3+h2+h1+g1, g1+g2+g3+h3+h1,
        a4+b4+b3+b2+a2+a1+b1, a2+a3+a4+b4+c4+c3+c2+b2+a1+b1+c1, b2+b3+b4+c4+d4+d3+d2+c2+b1+c1+d1, c2+c3+c4+d4+e4+e3+e2+d2+c1+d1+e1, d2+d3+d4+e4+f4+f3+f2+e2+d1+e1+f1, e2+e3+e4+f4+g4+g3+g2+f2+e1+f1+g1, f2+f3+f4+g4+h4+h3+h2+g2+f1+g1+h1, g2+g3+g4+h4+h2+g1+h1,
        a5+b5+b4+b3+a3+a2+b2, a3+a4+a5+b5+c5+c4+c3+b3+a2+b2+c2, b3+b4+b5+c5+d5+d4+d3+c3+b2+c2+d2, c3+c4+c5+d5+e5+e4+e3+d3+c2+d2+e2, d3+d4+d5+e5+f5+f4+f3+e3+d2+e2+f2, e3+e4+e5+f5+g5+g4+g2+f3+e2+f2+g3, f3+f4+f5+g5+h5+h4+h3+g3+f2+g2+h2, g3+g4+g5+h5+h3+g2+h2,
        a6+b6+b5+b4+a4+a3+b3, a4+a5+a6+b6+c6+c5+c4+b4+a3+b3+c3, b4+b5+b6+c6+d6+d5+d4+c4+b3+c3+d3, c4+c5+c6+d6+e6+e5+e4+d4+c3+d3+e3, d4+d5+d6+e6+f6+f5+f4+e4+d3+e3+f3, e4+e5+e6+f6+g6+g5+g4+f4+e3+f3+g3, f4+f5+f6+g6+h6+h5+h4+g4+f3+g3+h3, g4+g5+g6+h6+h4+g3+h3,
        a7+b7+b6+b5+a5+a4+b4, a5+a6+a7+b7+c7+c6+c5+b5+a4+b4+c4, b5+b6+b7+c7+d7+d6+d5+c5+b4+c4+d4, c5+c6+c7+d7+e7+e6+e5+d5+c4+d4+e4, d5+d6+d7+e7+f7+f6+f5+e5+d4+e4+f4, e5+e6+e7+f7+g7+g6+g5+f5+e4+f4+g4, f5+f6+f7+g7+h7+h6+h5+g5+f4+g4+h4, g5+g6+g7+h7+h5+g4+h4,
        a8+b8+b7+b6+a6+a5+b5, a6+a7+a8+b8+c8+c7+c6+b6+a5+b5+c5, b6+b7+b8+c8+d8+d7+d6+c6+b5+c5+d5, c6+c7+c8+d8+e8+e7+e6+d6+c5+d5+e5, d6+d7+d8+e8+f8+f7+f6+e6+d5+e5+f5, e6+e7+e8+f8+g8+g7+g6+f6+e5+f5+g5, f6+f7+f8+g8+h8+h7+h6+g6+f5+g5+h5, g6+g7+g8+h8+h6+g5+h5,
        b8+b7+a7+a6+b6, a7+a8+c8+c7+b7+a6+b6+c6, b7+b8+d8+d7+c7+b6+c6+d6, c7+c8+e8+e7+d7+c6+d6+e6, d7+d8+f8+f7+e7+d6+e6+f6, e7+e8+g8+g7+f7+e6+f6+g6, f7+f8+h8+h7+g7+f6+g6+h6, g7+g8+h7+g6+h6}}};

constexpr std::array<uint64_t, 64> knight_neighbors {
        b3+c2, a3+c3+d2, a2+b3+d3+e2, b2+c3+e3+f2, c2+d3+f3+g2, d2+e3+g3+h2, e2+f3+h3, f2+g3,
        b4+c3+c1, a4+c4+d3+d1, a1+a3+b4+d4+e3+e1, b1+b3+c4+e4+f3+f1, c1+c3+d4+f4+g3+g1, d1+d3+e4+g4+h3+h1, e1+e3+f4+h4, f1+f3+g4,
        b1+b5+c4+c2, c1+a1+a5+c5+d4+d2, d1+b1+a2+a4+b5+d5+e4+e2, e1+c1+b2+b4+c5+e5+f4+f2, f1+d1+c2+c4+d5+f5+g4+g2, g1+e1+d2+d4+e5+g5+h4+h2, h1+f1+e2+e4+f5+h5, g1+f2+f4+g5,
        b2+b6+c5+c3, c2+a2+a6+c6+d5+d3, d2+b2+a3+a5+b6+d6+e5+e3, e2+c2+b3+b5+c6+e6+f5+f3, f2+d2+c3+c5+d6+f6+g5+g3, g2+e2+d3+d5+e6+g6+h5+h3, h2+f2+e3+e5+f6+h6, g2+f3+f5+g6,
        b3+b7+c6+c4, c3+a3+a7+c7+d6+d4, d3+b3+a4+a6+b7+d7+e6+e4, e3+c3+b4+b6+c7+e7+f6+f4, f3+d3+c4+c6+d7+f7+g6+g4, g3+e3+d4+d6+e7+g7+h6+h4, h3+f3+e4+e6+f7+h7, g3+f4+f6+g7,
        b4+b8+c7+c5, c4+a4+a8+c8+d7+d5, d4+b4+a5+a7+b8+d8+e7+e5, e4+c4+b5+b7+c8+e8+f7+f5, f4+d4+c5+c7+d8+f8+g7+g5, g4+e4+d5+d7+e8+g8+h7+h5, h4+f4+e5+e7+f8+h8, g4+f5+f7+g8,
        b5+c8+c6, c5+a5+d8+d6, d5+b5+a6+a8+e8+e6, e5+c5+b6+b8+f8+f6, f5+d5+c6+c8+g8+g6, g5+e5+d6+d8+h8+h6, h5+f5+e6+e8, g5+f6+f8,
        b6+c7, c6+a6+d7, d6+b6+a7+e7, e6+c6+b7+f7, f6+d6+c7+g7, g6+e6+d7+h7, h6+f6+e7, g6+f7};

constexpr std::array<std::array <uint64_t, 64>, 2> pawn_neighbors {{{ 
        b2, a2+c2, b2+d2, c2+e2, d2+f2, e2+g2, f2+h2, g2, b3, a3+c3, b3+d3, c3+e3, d3+f3, e3+g3, f3+h3, g3, b4, a4+c4, b4+d4, c4+e4, d4+f4, e4+g4, f4+h4, g4, b5, a5+c5, b5+d5, c5+e5, d5+f5, e5+g5, f5+h5, g5,
        b6, a6+c6, b6+d6, c6+e6, d6+f6, e6+g6, f6+h6, g6, b7, a7+c7, b7+d7, c7+e7, d7+f7, e7+g7, f7+h7, g7, b8, a8+c8, b8+d8, c8+e8, d8+f8, e8+g8, f8+h8, g8, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL}, 
        {
        0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, b1, a1+c1, b1+d1, c1+e1, d1+f1, e1+g1, f1+h1, g1, b2, a2+c2, b2+d2, c2+e2, d2+f2, e2+g2, f2+h2, g2, b3, a3+c3, b3+d3, c3+e3, d3+f3, e3+g3, f3+h3, g3,
        b4, a4+c4, b4+d4, c4+e4, d4+f4, e4+g4, f4+h4, g4, b5, a5+c5, b5+d5, c5+e5, d5+f5, e5+g5, f5+h5, g5, b6, a6+c6, b6+d6, c6+e6, d6+f6, e6+g6, f6+h6, g6, b7, a7+c7, b7+d7, c7+e7, d7+f7, e7+g7, f7+h7, g7}}};

constexpr std::array<std::array <uint64_t, 65>, 2> pawn_movespace {{{ 
        0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, a3+a4, b3+b4, c3+c4, d3+d4, e3+e4, f3+f4, g3+g4, h3+h4, a4, b4, c4, d4, e4, f4, g4, h4, a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6, a7, b7, c7, d7, e7, f7, g7, h7, a8, b8, c8, d8, e8, f8, g8, h8,  0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL}, 
        {
        0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, a1, b1, c1, d1, e1, f1, g1, h1, a2, b2, c2, d2, e2, f2, g2, h2, a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4, a5, b5, c5, d5, e5, f5, g5, h5, a6+a5, b6+b5, c6+c5, d6+d5, e6+e5, f6+f5, g6+g5, h6+h5, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL}}};

constexpr std::array<uint64_t, 64> en_passant {
        0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, b4, a4+c4, b4+d4, c4+e4, d4+f4, e4+g4, f4+h4, g4, 
        b5, a5+c5, b5+d5, c5+e5, d5+f5, e5+g5, f5+h5, g5, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL};

const std::vector<std::vector<int>> mvv_lva {{0, 0, 25}, {}, {0, 0, 25, 26, 27, 28, 29, 30}, {0, 0, 19, 20, 21, 22, 23, 24}, {0, 0, 13, 14, 15, 16, 17, 18}, {0, 0, 7, 8, 9, 10, 11, 12}, {0, 0, 1, 2, 3, 4, 5, 6}};

constexpr int MATE       = 32000;        // checkmate at ply 0
constexpr int INF         = 32001;        // = MATE + 1; the "infinity" window bound
constexpr int MAX_PLY     = 128;          // your depth cap (40) + q-plies, with margin
constexpr int MATE_IN_MAX = MATE - MAX_PLY;   // 31872 — the mate-zone boundary

const uint64_t row_1_mask = a1+b1+c1+d1+e1+f1+g1+h1;
const uint64_t row_2_mask = a2+b2+c2+d2+e2+f2+g2+h2;
const uint64_t row_3_mask = a3+b3+c3+d3+e3+f3+g3+h3;
const uint64_t row_4_mask = a4+b4+c4+d4+e4+f4+g4+h4;
const uint64_t row_7_mask = a7+b7+c7+d7+e7+f7+g7+h7;
const uint64_t row_6_mask = a6+b6+c6+d6+e6+f6+g6+h6;
const uint64_t row_5_mask = a5+b5+c5+d5+e5+f5+g5+h5;
const uint64_t row_8_mask = a8+b8+c8+d8+e8+f8+g8+h8;

const uint64_t file_A_mask = a1+a2+a3+a4+a5+a6+a7+a8;
const uint64_t file_B_mask = b1+b2+b3+b4+b5+b6+b7+b8;
const uint64_t file_C_mask = c1+c2+c3+c4+c5+c6+c7+c8;
const uint64_t file_D_mask = d1+d2+d3+d4+d5+d6+d7+d8;
const uint64_t file_E_mask = e1+e2+e3+e4+e5+e6+e7+e8;
const uint64_t file_F_mask = f1+f2+f3+f4+f5+f6+f7+f8;
const uint64_t file_G_mask = g1+g2+g3+g4+g5+g6+g7+g8;
const uint64_t file_H_mask = h1+h2+h3+h4+h5+h6+h7+h8;

const uint64_t one_mask = ~0UL;
const uint64_t zero_mask = 0UL;

const uint64_t pit_on_off[2] {zero_mask, one_mask};

constexpr std::array<int, 8> piece_value {0, 0, 100, 320, 330, 500, 900, 1000}; // piece values for static exchange evaluation

constexpr std::array<std::string_view, 64> square_name = {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
                                "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
                                "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};

constexpr std::array<std::string_view, 14> uci_commands {"uci", "debug", "isready", "setoption", "register", "ucinewgame", "position", "go", "perft", "stop", "ponderhit", "quit", "test", "eval"};

}

enum enumPiece {
White,     // any white piece
Black,
Pawn,
Knight,
Bishop,
Rook,
Queen,
King,
status, // castling rights, en passant targets, ply,, etc....
};

struct  TScore {int mg, eg; };
// {mg, eg} per reachable-square count.
constexpr std::array<TScore, 9> mob_bonus_knight {{
        {-30,-35}, {-20,-24}, {-10,-14}, {-3,-5}, {3,3}, {8,9}, {12,13}, {15,16}, {17,18}
}};

constexpr std::array<TScore, 14> mob_bonus_bishop {{
        {-28,-32}, {-20,-23}, {-12,-15}, {-5,-7}, {1,0}, {6,6}, {11,12}, {15,16},
        {18,20}, {21,23}, {23,26}, {25,28}, {27,30}, {28,31}
}};

constexpr std::array<TScore, 15> mob_bonus_rook {{
        {-24,-34}, {-17,-24}, {-11,-15}, {-6,-7}, {-2,0}, {2,7}, {5,13}, {8,18},
        {11,23}, {13,27}, {15,31}, {16,34}, {17,37}, {18,39}, {19,41}
}};

constexpr std::array<TScore, 28> mob_bonus_queen {{
        {-14,-20}, {-11,-16}, {-9,-13}, {-7,-10}, {-5,-7}, {-3,-4}, {-1,-2}, {1,0},
        {2,2}, {3,4}, {4,5}, {5,7}, {6,8}, {7,9}, {8,10}, {8,11}, {9,12}, {9,13},
        {10,14}, {10,14}, {11,15}, {11,16}, {12,16}, {12,17}, {12,17}, {13,18}, {13,18}, {13,18}
}};

static const std::array<int, 8> king_attack_weight {0, 0, 0, 2, 2, 3, 5, 0};

// passed-pawn bonus by RELATIVE rank (0 = own back rank … 6 = 7th/about to promote). eg >> mg.
constexpr std::array<TScore, 8> passed_bonus {{
    {0,0}, {0,5}, {5,15}, {12,30}, {22,55}, {38,90}, {60,140}, {0,0}
}};

constexpr TScore isolated_penalty { -12, -16 };   // per isolated pawn
constexpr TScore doubled_penalty  { -10, -22 };   // per EXTRA pawn on a file (doubled worse in the ending)

constexpr TScore bishop_pair_bonus { 20, 35 };   // both bishops cover both colour complexes; shine in the open ending

constexpr TScore rook_open { 25, 15 };   // fully open file (no pawns of either colour)
constexpr TScore rook_semi { 12,  8 };   // semi-open (no OWN pawns; enemy pawns allowed)

constexpr int tempo = 15;   // side-to-move bonus (flat — not phase-dependent)

constexpr std::array<std::string_view, 8> algebra_pieces {"", "", "", "N", "B", "R", "Q", "K"};

constexpr std::string_view engine_version = "7.0.1";   // the one place the version lives: id name, tag, README

const uint64_t white_king_side = e1+f1+g1;
const uint64_t white_queen_side = c1+d1+e1;
const uint64_t black_king_side = e8+f8+g8;
const uint64_t black_queen_side = c8+d8+e8;
constexpr uint64_t ep_castling_mask = 9295710006374498433UL; // status bits changed for castling(0/7/56/63) + e.p. rows(3,6) → Zobrist update
constexpr uint64_t check_flag = 16777216UL; // status bit 24 = "side to move in check"
constexpr uint64_t halfmove_clock_mask = 4261412864UL; // status bits 25–31  = fifty-move counter

constexpr uint32_t from_square_mask = 64512; // from-square, bits 10–15
constexpr uint32_t to_square_mask = 1008; // to-square, bits 4-9

static_assert(my_const::square_name.size()     == 64, "square_name: one per board square");
static_assert(my_const::uci_commands.size()    == 14, "uci_commands: keep in sync with the dispatch");
static_assert(my_const::diagonals.size()       == 28, "diagonals: 26 rays + all-board + empty");
static_assert(my_const::king_neighbors.size()  == 64, "king_neighbors: one mask per square");
static_assert(my_const::knight_neighbors.size()== 64, "knight_neighbors: one mask per square");


#endif