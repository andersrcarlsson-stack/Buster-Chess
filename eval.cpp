#include "global_header.h"
#include "support_files.h"    // Chess, evaluation decl (+ make, if referenced)
#include "global_constants.h" // my_const::*
#include "piece_table.h"      // piece_square_tables
#include "tables.h"           // Lookup
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null
#include "eval.h"

int evaluation (const Chess & Board) { // this pit is only for telling the evaluation function which player is the maximizer

    std::array<int, 2> valuation {};
    uint64_t evaluated_bit, set_bitboard;
    int score;

    // --- draw recognition: material that can't force mate IS a draw -> return 0.
    // Removes the "+bishop" lie (KBvK etc.), and it's a speed win too (skips the whole eval).
    if (Board.bitboard[2] == 0 && Board.bitboard[5] == 0 && Board.bitboard[6] == 0) {   // no pawns, rooks, queens
        int wm = __builtin_popcountl(Board.bitboard[0] & (Board.bitboard[3] | Board.bitboard[4])); // white minors
        int bm = __builtin_popcountl(Board.bitboard[1] & (Board.bitboard[3] | Board.bitboard[4])); // black minors
        if (wm <= 1 && bm <= 1) return 0;                     // K(m) vs K(m): KvK, KBvK, KNvK, Km-vs-Km
        if (Board.bitboard[4] == 0 && ((wm == 2 && bm == 0) || (bm == 2 && wm == 0)))
            return 0;                                         // KNNvK (all-knight, 2 vs bare)
    }

    int phase = __builtin_popcountl(Board.bitboard[3] | Board.bitboard[4]) + 
                __builtin_popcountl(Board.bitboard[5] ) * 2 + 
                __builtin_popcountl(Board.bitboard[6] ) * 4;  // numbers of n, b, r and q on the board with weights 1, 1, 2, 4

    // Piece Value and Piece Square Values combined for one loop 
        for (int i = 0; i < 6; ++i) {
            set_bitboard = Board.bitboard[i + 2];
            while (set_bitboard) {
                evaluated_bit = set_bitboard & (~set_bitboard + 1UL); // take the rightmost bit in set_bitboard and make it "evaluation_bit"
                set_bitboard &= set_bitboard - 1UL; // remove the "evaluation_bit" from set_bitboard 
                valuation[0] += (piece_square_tables[0][i][Board.pit][__builtin_ctzl(evaluated_bit)] * !!(Board.bitboard[Board.pit] & evaluated_bit)) - 
                        (piece_square_tables[0][i][!Board.pit][__builtin_ctzl(evaluated_bit)] * !!(Board.bitboard[!Board.pit] & evaluated_bit));
                valuation[1] += (piece_square_tables[1][i][Board.pit][__builtin_ctzl(evaluated_bit)] * !!(Board.bitboard[Board.pit] & evaluated_bit)) - 
                        (piece_square_tables[1][i][!Board.pit][__builtin_ctzl(evaluated_bit)] * !!(Board.bitboard[!Board.pit] & evaluated_bit));
            }
        }
    
        // mobility - also prepares for king safety .... 

        std::array<std::array<uint64_t, 8>, 2> can_reach {};
        std::array<uint64_t, 2> mob_area;
        uint64_t occ = Board.bitboard[0] | Board.bitboard[1], atk, piece_bb;
        std::array<TScore, 2> mob {};   // {} zero-inits both .mg and .eg for both colours
        int ev_square, count;

        // generate a bitboard "can_reach" for all sliders - both colors !

        can_reach[0][Pawn] = (((Board.bitboard[0] & Board.bitboard[Pawn]) & ~my_const::file_A_mask) << 7 | ((Board.bitboard[0] & Board.bitboard[Pawn]) & ~my_const::file_H_mask) << 9 );
        can_reach[1][Pawn] = (((Board.bitboard[1] & Board.bitboard[Pawn]) & ~my_const::file_A_mask) >> 9 | ((Board.bitboard[1] & Board.bitboard[Pawn]) & ~my_const::file_H_mask) >> 7 );

        for (int c = 0; c < 2; ++c) { // the evaluation need both colors ability to reach

            mob_area[c] = ~Board.bitboard[c] & ~can_reach[1 - c][Pawn];

            piece_bb =  Board.bitboard[c] & Board.bitboard[Knight]; // all knights from "c" (pit)
            while (piece_bb) {  // loop all Knights from color "c"
                ev_square = __builtin_ctzll(piece_bb);
                atk = my_const::knight_neighbors[ev_square]; 
                can_reach[c][Knight] |= atk; // union → king safety later (raw)
                count = __builtin_popcountll(atk & mob_area[c]);
                mob[c].mg += mob_bonus_knight[count].mg;
                mob[c].eg += mob_bonus_knight[count].eg;
                piece_bb &= piece_bb - 1UL; // clear lowest set bit from piece_bb
            }

            piece_bb =  Board.bitboard[c] & Board.bitboard[Bishop]; // all bishops from "c" (pit)
            while (piece_bb) {  // loop all Bishops from color "c"
                ev_square = __builtin_ctzll(piece_bb);
                atk = Lookup.diag.attacks(ev_square, occ); 
                can_reach[c][Bishop] |= atk; // union → king safety later (raw)
                count = __builtin_popcountll(atk & mob_area[c]);
                mob[c].mg += mob_bonus_bishop[count].mg;
                mob[c].eg += mob_bonus_bishop[count].eg;                
                piece_bb &= piece_bb - 1UL; // clear lowest set bit from piece_bb
            }
            
            piece_bb =  Board.bitboard[c] & Board.bitboard[Rook];
            while (piece_bb) {  // loop all Rooks from color "c"
                ev_square = __builtin_ctzll(piece_bb);
                atk = Lookup.rank_file.attacks(ev_square, occ); 
                can_reach[c][Rook] |= atk; // union → king safety later (raw)
                count = __builtin_popcountll(atk & mob_area[c]);
                mob[c].mg += mob_bonus_rook[count].mg;
                mob[c].eg += mob_bonus_rook[count].eg;
                piece_bb &= piece_bb - 1UL; // clear lowest set bit from piece_bb
            }

            piece_bb =  Board.bitboard[c] & Board.bitboard[Queen];
            while (piece_bb) {  // loop all Queens from color "c"
                ev_square = __builtin_ctzll(piece_bb); 
                atk = Lookup.diag.attacks(ev_square, occ) | Lookup.rank_file.attacks(ev_square, occ); 
                can_reach[c][Queen] |= atk; // union → king safety later (raw)
                count = __builtin_popcountll(atk & mob_area[c]);
                mob[c].mg += mob_bonus_queen[count].mg;
                mob[c].eg += mob_bonus_queen[count].eg;
                piece_bb &= piece_bb - 1UL; // clear lowest set bit from piece_bb
            }
        }

        valuation[0] += mob[Board.pit].mg - mob[!Board.pit].mg;   // mobility, side-to-move POV, midgame
        valuation[1] += mob[Board.pit].eg - mob[!Board.pit].eg;   // mobility, endgame

        // --- king safety: weighted enemy attacks into each king's zone -> non-linear danger ---
        // attacker weight by piece type, indexed by enumPiece: N=2, B=2, R=3, Q=5 (0 for the rest)
        std::array<int, 2> king_danger {};

        for (int c = 0; c < 2; ++c) {
            int king_sq   = __builtin_ctzll(Board.bitboard[King] & Board.bitboard[c]);
            uint64_t zone = my_const::king_safety[c][king_sq];
            int units = king_attack_weight[Knight] * __builtin_popcountll(can_reach[1 - c][Knight] & zone)
                      + king_attack_weight[Bishop] * __builtin_popcountll(can_reach[1 - c][Bishop] & zone)
                      + king_attack_weight[Rook]   * __builtin_popcountll(can_reach[1 - c][Rook]   & zone)
                      + king_attack_weight[Queen]  * __builtin_popcountll(can_reach[1 - c][Queen]  & zone);
            king_danger[c] = std::min(units * units / 4, 500);   // quadratic, capped (cp). DIV=4, CAP=500 = the tunables
        }

        // own king danger is bad, enemy king danger is good (side-to-move POV). Midgame-only term.
        valuation[0] += king_danger[!Board.pit] - king_danger[Board.pit];

        // --- passed pawns ---
        std::array<TScore, 2> passed {};
        for (int c = 0; c < 2; ++c) {
            uint64_t enemy_pawns = Board.bitboard[Pawn] & Board.bitboard[1 - c];
            uint64_t bb          = Board.bitboard[Pawn] & Board.bitboard[c];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                if ((Lookup.passed_pawn[c][sq] & enemy_pawns) == 0) {          // passed!
                    int rel_rank = (c == White) ? (sq >> 3) : (7 - (sq >> 3)); // advance toward promotion
                    passed[c].mg += passed_bonus[rel_rank].mg;
                    passed[c].eg += passed_bonus[rel_rank].eg;
                }
                bb &= bb - 1UL;
            }
        }
        valuation[0] += passed[Board.pit].mg - passed[!Board.pit].mg;
        valuation[1] += passed[Board.pit].eg - passed[!Board.pit].eg;   // eg carries it

        // --- isolated & doubled pawns (per file) ---
        std::array<TScore, 2> pstruct {};
        for (int c = 0; c < 2; ++c) {
            uint64_t own_pawns = Board.bitboard[Pawn] & Board.bitboard[c];
            for (int f = 0; f < 8; ++f) {
                uint64_t file_bb = 0x0101010101010101ULL << f;
                int n = __builtin_popcountll(file_bb & own_pawns);
                if (n == 0) continue;
                uint64_t adj = ((f > 0) ? (0x0101010101010101ULL << (f - 1)) : 0)
                             | ((f < 7) ? (0x0101010101010101ULL << (f + 1)) : 0);
                if (n > 1) {                                   // doubled: penalise each extra pawn
                    pstruct[c].mg += doubled_penalty.mg * (n - 1);
                    pstruct[c].eg += doubled_penalty.eg * (n - 1);
                }
                if ((adj & own_pawns) == 0) {                  // isolated: no friendly pawn on either neighbour
                    pstruct[c].mg += isolated_penalty.mg * n;
                    pstruct[c].eg += isolated_penalty.eg * n;
                }
            }
        }
        valuation[0] += pstruct[Board.pit].mg - pstruct[!Board.pit].mg;
        valuation[1] += pstruct[Board.pit].eg - pstruct[!Board.pit].eg;

        // --- bishop pair ---
        std::array<TScore, 2> bishoppair {};
        for (int c = 0; c < 2; ++c)
            if (__builtin_popcountll(Board.bitboard[Bishop] & Board.bitboard[c]) >= 2) {
                bishoppair[c].mg += bishop_pair_bonus.mg;
                bishoppair[c].eg += bishop_pair_bonus.eg;
            }
        valuation[0] += bishoppair[Board.pit].mg - bishoppair[!Board.pit].mg;
        valuation[1] += bishoppair[Board.pit].eg - bishoppair[!Board.pit].eg;

        // --- rook on open / semi-open file ---
        std::array<TScore, 2> rookfile {};
        uint64_t all_pawns = Board.bitboard[Pawn];
        for (int c = 0; c < 2; ++c) {
            uint64_t own_pawns = Board.bitboard[Pawn] & Board.bitboard[c];
            uint64_t bb        = Board.bitboard[Rook] & Board.bitboard[c];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                uint64_t file_bb = 0x0101010101010101ULL << (sq & 7);
                if ((file_bb & all_pawns) == 0) {              // open
                    rookfile[c].mg += rook_open.mg; rookfile[c].eg += rook_open.eg;
                } else if ((file_bb & own_pawns) == 0) {       // semi-open
                    rookfile[c].mg += rook_semi.mg; rookfile[c].eg += rook_semi.eg;
                }
                bb &= bb - 1UL;
            }
        }
        valuation[0] += rookfile[Board.pit].mg - rookfile[!Board.pit].mg;
        valuation[1] += rookfile[Board.pit].eg - rookfile[!Board.pit].eg;

        // --- draw scaling (soft): opposite-coloured bishops are drawish when material is level.
        // Pure OCB + pawn diff <= 1 -> downscale the eg component toward 0 (taper, NOT a hard cut). Bigger pawn
        // advantages are left alone so a genuinely winning OCB isn't drawn away — the search converts those.
        if (Board.bitboard[3] == 0 && Board.bitboard[5] == 0 && Board.bitboard[6] == 0   // no knights/rooks/queens
            && __builtin_popcountl(Board.bitboard[0] & Board.bitboard[4]) == 1  // one white bishop
            && __builtin_popcountl(Board.bitboard[1] & Board.bitboard[4]) == 1) {  // one black bishop
            constexpr uint64_t LIGHT = 0x55AA55AA55AA55AAULL;
            bool wb_light = Board.bitboard[0] & Board.bitboard[4] & LIGHT;
            bool bb_light = Board.bitboard[1] & Board.bitboard[4] & LIGHT;
            int pawn_diff = __builtin_popcountl(Board.bitboard[0] & Board.bitboard[2])
                          - __builtin_popcountl(Board.bitboard[1] & Board.bitboard[2]);
            if (wb_light != bb_light && pawn_diff >= -1 && pawn_diff <= 1)  // OCB, ~level material
                valuation[1] = valuation[1] * 8 / 64; // heavy downscale (~1/8)
        }

        // --- wrong-coloured rook pawn + bishop.
        // K(B) + rook-pawn(s) vs K is a fortress when nothing controls the promotion
        // square and the defending king holds it: the pawns can only queen on that one
        // square. Four guards — AT MOST one bishop (so never KBBvK/KBNvK), and if there
        // is one it must be the wrong colour; the defender has no bishop; every pawn on
        // the SAME edge file; the defender has no pawns.
        // The bishop is deliberately optional. Requiring exactly one made LOSING it an
        // escape from the scaling (63 -> 209), so the search was paid to sacrifice the
        // piece — and the score climbed with depth instead of converging.
        // Tapered, not a hard 0: the defending king can leave the corner and come back,
        // and a cliff at that boundary would oscillate the score across a move that does
        // not change the result.
        if (Board.bitboard[Knight] == 0 && Board.bitboard[Rook] == 0 && Board.bitboard[Queen] == 0) {
            constexpr int fortress_scale[4] = { 4, 4, 16, 32 };   // by king distance, /64
            for (int c = 0; c < 2; ++c) {
                const uint64_t strong = Board.bitboard[c], weak = Board.bitboard[1 - c];
                const int nb = __builtin_popcountl(Board.bitboard[Bishop] & strong);
                if (nb > 1) continue;   // >1 bishop -> can cover both colours
                if (Board.bitboard[Bishop] & weak) continue;
                const uint64_t sp = Board.bitboard[Pawn] & strong;
                if (sp == 0 || (Board.bitboard[Pawn] & weak)) continue;
                int pf;
                if      ((sp & ~my_const::file_A_mask) == 0) pf = 0; // all pawns on the a-file
                else if ((sp & ~my_const::file_H_mask) == 0) pf = 7; // all pawns on the h-file
                else continue;
                const int prom = (c == White) ? (56 + pf) : pf; // promotion square
                constexpr uint64_t LIGHT = 0x55AA55AA55AA55AAULL;
                if (nb == 1 && ((Board.bitboard[Bishop] & strong & LIGHT) != 0)
                            == (((1ULL << prom) & LIGHT) != 0)) continue;  // bishop covers it -> winnable
                const int wk = __builtin_ctzl(Board.bitboard[King] & weak);
                const int d  = std::max(std::abs((wk >> 3) - (prom >> 3)),
                                        std::abs((wk & 7)  - (prom & 7)));
                if (d <= 3) {  // scale BOTH phases: the fortress is not a
                    valuation[0] = valuation[0] * fortress_scale[d] / 64; // "winning eg, drawish
                    valuation[1] = valuation[1] * fortress_scale[d] / 64; //  mg" — it is just drawn
                }
            }
        }

        // --- pawnless small edge. The side the eval favours has NO pawns and is up
        // at most a minor piece -> it cannot force mate: KRvKB/KRvKN are textbook draws, a lone minor
        // cannot win against pawns, KRNvKR-type edges are drawish. Stockfish's classic material rule, in
        // pawn units (minor 3, rook 5, queen 9). Only ever damps a PAWNLESS side's advantage — if the
        // pawn side is winning, the eval favours it and the rule does not fire. KBBvKN is the exception
        // (two bishops beat a knight). Both phases scaled, as in the fortress rule: at Buster's phase
        // these positions still carry ~20% mg weight, which would otherwise leak through the blend.
        {
            const int c = (valuation[1] > 0) ? Board.pit : !Board.pit;        // the side the eg favours
            const uint64_t strong = Board.bitboard[c], weak = Board.bitboard[!c];
            if ((strong & Board.bitboard[Pawn]) == 0) {
                auto npm = [&](uint64_t side) {                               // non-pawn material, pawn units
                    return 3 * __builtin_popcountl(side & (Board.bitboard[Knight] | Board.bitboard[Bishop]))
                         + 5 * __builtin_popcountl(side & Board.bitboard[Rook])
                         + 9 * __builtin_popcountl(side & Board.bitboard[Queen]);
                };
                const int ns = npm(strong), nw = npm(weak);
                const bool kbbkn = ns == 6 && __builtin_popcountl(strong & Board.bitboard[Bishop]) == 2
                                && nw == 3 && (weak & Board.bitboard[Knight]);
                if (ns - nw <= 3 && !kbbkn) {
                    const int scale = ns < 5 ? 0 : (nw <= 3 ? 4 : 14);          // /64
                    valuation[0] = valuation[0] * scale / 64;
                    valuation[1] = valuation[1] * scale / 64;
                }
            }
        }

        // phase blend + tempo bonus for side to move.....
        score = (valuation[0] * phase + valuation[1] * (24 - phase)) / 24 + tempo;

    return score;
}
