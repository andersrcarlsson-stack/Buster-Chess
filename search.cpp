#include "global_header.h"
#include "support_files.h"   // Chess, MoveList, Position_List; decls for search/alpha_beta/q_search + make/unmake/make_null/unmake_null/generate_moves/make_new_state/evaluation
#include "global_constants.h"// my_const:: (MATE bounds, masks)
#include "piece_table.h"
#include "search_state.h"    // Stop, Tune, Killers, History, SEE
#include "tt.h"              // zobrist  (40 refs — the hot one)
#include "tables.h"          // Lookup, Move
#include "uci.h"             // uci  (per-iteration info output)
#include "movegen.h"
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null
#include "search.h"
#include "eval.h"
#include "movepicker.h"

// Principal variation from the TT: follow each position's stored best move from the root.
// Every move is checked legal before it's played; stops at a TT miss, an illegal move, or max_len
// (the cap also ends repetition cycles). Works on a copy — make_new_state touches no global state.
static std::string tt_pv(Chess b, uint16_t m, int max_len) {
    std::string pv;
    for (int n = 0; n < max_len; ++n) {
        generate_moves(b);
        if (std::find(b.possible_moves.begin(), b.possible_moves.end(), m) == b.possible_moves.end()) break;
        pv += uci.decode_buster_move(m) + ' ';
        make_new_state(b, m);
        tp_table* e = zobrist.probe(b.tpt.zobrist_key_64);
        if (!e or e->move == 0) break;
        m = e->move;
    }
    return pv;
}

// this is the detached thread Main Search Function
// it is centered around alpha_beta call that returns struct best with int value and uint16_t move
// it is also the iterative deep loop which is the mechanism that completes the search function (all depths)
// it is also the main communicator to uci via prining to terminal -- part results and best move
void search (Chess Board, int depth, int soft_time, int hard_time)  {

    // clock_t start = clock();
    auto start = std::chrono::steady_clock::now();
    auto timeisup = start + std::chrono::milliseconds(hard_time);

    int max_found; // local variable - protects max_found if stop in middle of alpha_beta - return
    uint16_t move = Board.possible_moves.front(); // start with a legal move so an early abort still returns something legal
    
    int prev = 0; 
    for (int i = 1; i <= depth; ++i){ // iterative deep

        int max_q_depth = 0;

        // alpha-beta search with aspiration window, starting with a wide window and narrowing it if the result is outside the window
        best result;
        int delta = Tune.asp_delta;
        int alpha = -my_const::INF, beta = my_const::INF;
        if (i >= Tune.asp_min_depth and std::abs(prev) < my_const::MATE_IN_MAX) {
            alpha = prev - delta;                    // narrow band around last score
            beta  = prev + delta;
        }
        while (true) {
            result = alpha_beta(Board, alpha, beta, i, 0, max_q_depth, timeisup);
            if (result.uci_stop) break;
            if (result.value <= alpha) {             // fail low  -> relax alpha, keep beta
                alpha = std::max(result.value - delta, -my_const::INF);
                delta += delta;
            } else if (result.value >= beta) {       // fail high -> relax beta, keep alpha
                beta = std::min(result.value + delta, my_const::INF);
                delta += delta;
            } else break;                            // landed inside -> accept
        }

        // if uci.stop is set in any search or by uci - bypass the update just below and send the uci bestmove
        if (result.uci_stop) break; 

        max_found = result.value; 
        move = result.move;
        prev = result.value; // save the last score for the next iteration's aspiration window

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        long long nps = Stop.nodes * 1000 / std::max<long long>(elapsed.count(), 1);
        std::string score;
        if (std::abs(max_found) >= my_const::MATE_IN_MAX) {
            int mate_plies = my_const::MATE - std::abs(max_found);   // plies to mate (>= 0)
            int mate_moves = std::max(1, (mate_plies + 1) / 2);      // plies -> moves, floor at 1
            score = "mate " + std::to_string(max_found > 0 ? mate_moves : -mate_moves);
        } else score = "cp " + std::to_string(max_found);

        std::cout << "info depth " << i
                  << " seldepth "  << std::max(i, max_q_depth)
                  << " score "     << score
                  << " nodes "     << Stop.nodes
                  << " nps "       << nps
                  << " hashfull "  << zobrist.hashfull()
                  << " time "      << elapsed.count()
                  << " pv "        << tt_pv(Board, move, i) << std::endl;

        if (elapsed.count() >= soft_time) break;   // next iteration wouldn't finish - keep what we have
    }

    // Safety net: never emit an illegal move. Board is restored to the root here;
    // if the search's chosen move isn't among the root's legal moves (e.g. move==0
    // from an aborted/mate-edge iteration), fall back to a guaranteed-legal move.
    generate_moves(Board);
    bool legal = false;
    for (uint16_t m : Board.possible_moves) if (m == move) { legal = true; break; }
    if (!legal && Board.possible_moves.size() > 0) move = Board.possible_moves.front();

    std::cout << "info " << "string " << uci.decode_buster_to_algebraic(Board, move) << std::endl;
    std::cout << "bestmove " << uci.decode_buster_move(move) << std::endl;
}

best alpha_beta (Chess & Board, int alpha, int beta, int depth, int ply, int & max_q_depth, std::chrono::steady_clock::time_point timeisup, bool can_null) {
    
    ++Stop.nodes;
    int alphaOrig = alpha;
    
    // threefold repetition checker
    if ((zobrist.position_list.size() >=2) and (ply > 0) and (std::find(zobrist.position_list.begin(), zobrist.position_list.end() - 1, Board.tpt.zobrist_key_64))!= zobrist.position_list.end() - 1) return {0,0};

    if (Board.possible_moves.size() == 0) {
        if (Board.bitboard[status] & check_flag) return {-my_const::MATE + ply, 0};
        else return {0, 0}; }
    // 50-move-rule....
    if ((ply > 0) and (_pext_u64(Board.bitboard[status], halfmove_clock_mask) > 99)) return {0, 0};
    if (depth == 0 and Board.possible_captures.size() != 0) return q_search(Board, alpha, beta, ply, max_q_depth,timeisup);
    // if Board is found in zobrist.position_list: return contempt value....in result.value....
    else if (depth == 0) return {evaluation(Board), 0U};

    uint64_t parent_key = Board.tpt.zobrist_key_64;   // Board gets mutated below - save its key
    tp_table* hit = zobrist.probe(parent_key);
    uint16_t hash_move = hit ? hit->move : 0;   // capture the hash move on a hit
    if (hit and hit->depth >= depth) {
        if (hit->flag == 'e') return {hit->value, hit->move};
        if (hit->flag == 'l' and hit->value >= beta)  return {hit->value, hit->move};
        if (hit->flag == 'u' and hit->value <= alpha) return {hit->value, hit->move};
        // else: fall through and search normally
    }

    bool node_in_check = Board.bitboard[status] & check_flag;  // this node in check? (don't reduce evasions)
    // static eval, computed once for both reverse-futility (here) and futility (in the loop)
    int static_eval = (depth <= std::max(Tune.futility_depth, Tune.rfp_depth) and !node_in_check)
                      ? evaluation(Board) : 0;

    // Reverse futility / static null-move: so far ABOVE beta that a full per-ply margin can't
    // pull us under -> assume fail-high, cut without searching. Mirror of the futility below.
    if (depth <= Tune.rfp_depth and !node_in_check
        and beta < my_const::MATE_IN_MAX and beta > -my_const::MATE_IN_MAX
        and static_eval - Tune.rfp_margin * depth >= beta)
        return {static_eval, 0};

    int good_caps = SEE.SEE_Sort(Board);

    best result {-my_const::INF, 0U};
    MoveList moves = Board.possible_moves;   // copy - make() clears Board's list (now a stack copy)
    result.move = moves.front();   // never leave the best move unset -> a fold-nothing node can't return move 0
    MoveList captures = Board.possible_captures;   // save

    // move ordering - hash move first: the best move found here last time (TT), rotated (not swapped)
    // to the front so the SEE capture order behind it survives intact
    int insert = Board.possible_captures.size();   // first quiet slot - captures keep their lead
    bool hash_at_front = false;
    if (hash_move) {
        auto it = std::find(moves.begin(), moves.end(), hash_move);
        if (it != moves.end()) {
            std::rotate(moves.begin(), it, it + 1); // hash move to front; everything else keeps its order
            hash_at_front = true; // a hash move was actually rotated to ordinal 0
            if (!(hash_move & 4)) ++insert; // a quiet hash move at [0] pushes the captures back one slot
        }
    }

    int lose_start = insert - captures.size() + good_caps;   // cap_base + good_caps

    int pp = (ply > 0) ? Line.piece(ply - 1) : 0;   // previous move's (piece, to) — continuation context
    int pt = (ply > 0) ? Line.to(ply - 1)    : 0;

    // move ordering - killers: the two quiet cutoff-movers from this ply, placed right after the captures
    for (int s = 0; s < Killer_table::slots; ++s) {
        uint16_t killer = Killers.get(ply, s);
        if (!killer) continue;                     // empty slot
        auto it = std::find(moves.begin() + insert, moves.end(), killer);   // search the quiet tail only
        if (it != moves.end()) { std::iter_swap(moves.begin() + insert, it); ++insert; }
    }

    MovePicker mp(hash_at_front, lose_start, insert); // Construct the picker, passing the values already computed
    mp.snapshot_quiets(moves, insert, Board.pit, Board, pp, pt);   // pin history+conthist at node entry
        
        // ---- Null-move pruning ---------------------------------------------------------------
    // Pass the move; if the opponent STILL can't reach beta with a shallower, null-window search,
    // this node fails high anyway -> cut off without searching our moves. Each guard is a real
    // failure mode:
    //   can_null       - the parent didn't just null (no two passes in a row = same position, garbage)
    //   not in check   - you can't legally pass out of check, and it's unsound
    //   depth >= 3     - enough left for the reduced search to mean anything (and depth-1-R >= 0)
    //   non-pawn piece - the side to move has a N/B/R/Q; else zugzwang makes "pass" a lie
    //   beta is a real bound, not a mate score (abs >= MATE_IN_MAX)
    if (can_null and ply > 0
        and !(Board.bitboard[status] & check_flag)
        and depth >= 3 and depth >= Tune.null_move_r + 1
        and ((Board.bitboard[Knight] | Board.bitboard[Bishop] | Board.bitboard[Rook] | Board.bitboard[Queen]) & Board.bitboard[Board.pit])
        and beta < my_const::MATE_IN_MAX) { 

        const int R = Tune.null_move_r;
        Undo undo;
        make_null(Board, undo);
        generate_moves(Board);  // the pass-position needs its own move list
        Line.set(ply, 0, 0);   // a null move has no piece — the child sees "no counter"
        best nm = alpha_beta(Board, -beta, -beta + 1, depth - 1 - R, ply + 1, max_q_depth, timeisup, false);
        unmake_null(Board, undo);                     // Board's own move lists are restored from the locals at the tail

        if (nm.uci_stop) { result.uci_stop = true; return result; }   // aborted -> discard, don't cut off
        int null_score = -nm.value;
        if (null_score >= beta) {
            if (null_score >= my_const::MATE_IN_MAX) null_score = beta;   // never propagate a null-move-induced "mate"
            return {null_score, 0};
        }
    }
    
    int quiets_seen = 0;   // late history-quiets reached at this node (drives LMP)
           
    for (int i = 0; i < moves.size(); ++i) {
        if (i >= insert) mp.pick_next_quiet(moves, i);   // lazy: best quiet into slot i
        uint16_t move = moves[i];
        MoveStage st = mp.stage(i, move);   // once per move
        // break and return last result at uci requested "stop" and at time budget constraint
        if ((Stop.node_limit and Stop.nodes >= Stop.node_limit)
            or (((Stop.nodes & 2047) == 0) and (std::chrono::steady_clock::now() > timeisup))
            or Stop.stop.load(std::memory_order_relaxed)) {
            result.uci_stop = true;
            break;
        }

        // Futility pruning: skip quiet, non-promoting moves that provably can't raise alpha.
        bool futile_quiet  = !(move & 4); // quiet (existing)
        bool futile_loscap = st == MoveStage::LosingCapture;   // SEE<0 capture (reuses the LMR boundary)
        if (depth <= Tune.futility_depth and i > 0 and !node_in_check
            and alpha < my_const::MATE_IN_MAX and alpha > -my_const::MATE_IN_MAX
            and !(move & 8) // never a promotion
            and (futile_quiet or futile_loscap)
            and static_eval + Tune.futility_margin * depth <= alpha)
            continue;

        // Late-move pruning: near the frontier, once enough quiets have been tried none of the rest
        // beat alpha — skip them outright (no make, no subtree). Quiets are contiguous at the tail,
        // so once the count trips, every remaining move is a lower-ranked quiet -> break.
        bool is_late_quiet = st == MoveStage::Quiet;
        if (is_late_quiet and depth <= Tune.lmp_depth and !node_in_check
            and alpha < my_const::MATE_IN_MAX and alpha > -my_const::MATE_IN_MAX
            and !(move & 8)                                  // never a promotion (mirror futility)
            and quiets_seen >= 3 + depth * depth)
            break;
        if (is_late_quiet) ++quiets_seen;

        int mover = Board.mailbox[_pext_u32(move, from_square_mask)];  // read before make vacates 'from'
        Line.set(ply, mover, _pext_u32(move, to_square_mask));   // record this move for the child's context

        // Make move (in place), recurse, then unmake to restore Board exactly
        Undo undo;
        make (Board, move, undo);
        zobrist.prefetch(Board.tpt.zobrist_key_64); // hide the child's TT miss under movegen
        generate_moves(Board);
        zobrist.position_list.push_back(Board.tpt.zobrist_key_64); // detect repetition - add latest - have to be after make

        bool gives_check = Board.bitboard[status] & check_flag;   // our move gave check → search that line deeper
        int  extension   = gives_check ? Tune.check_ext : 0;

        // ---- Late Move Reductions -----------------------------------------------------------
        // Late quiet moves (past the hash move / captures / killers = index >= insert) that give no
        // check are probably bad: search them a ply shallower. If one beats alpha it may be good after
        // all -> re-search at full depth. Skipped when shallow, in check, or the move gives check.
        // including LUT r(depth, movenumber) ≈ base + ln(depth)·ln(movenumber) / div      (base≈0.75, div≈2.25)
        int reduction = 0;
        if (depth >= 3 and !node_in_check and !gives_check
            and (st == MoveStage::Quiet or st == MoveStage::LosingCapture)) {
            reduction = Tune.lmr_r(depth, i);
            if (reduction > depth - 2) reduction = depth - 2;   // keep depth-1-reduction >= 1
        }

        best valuation;
        if (reduction) {
            MoveList child_moves = Board.possible_moves;      // save the child's freshly-generated lists
            MoveList child_caps  = Board.possible_captures;   //   (generate_moves is deterministic)
            valuation = alpha_beta (Board, -beta, -alpha, depth - 1 - reduction, ply + 1, max_q_depth, timeisup);
            if (!valuation.uci_stop and -valuation.value > alpha) {  // surprised us -> verify at full depth
                Board.possible_moves    = child_moves;        // restore instead of regenerating — byte-identical
                Board.possible_captures = child_caps;         //   list, but skips the attack-map + pin rebuild (the fee)
                valuation = alpha_beta (Board, -beta, -alpha, depth - 1, ply + 1, max_q_depth, timeisup);
            }
        } else {
            valuation = alpha_beta (Board, -beta, -alpha, depth - 1 + extension, ply + 1, max_q_depth, timeisup);
        }

        unmake (Board, move, undo);   // Board is the parent again from here on
        zobrist.position_list.pop_back(); // pop out the latest pos from repetion detection

        if (valuation.uci_stop) { result.uci_stop = true; break; }   // abort cleanly: don't use/store a half-searched value
        int value = -valuation.value;

        if (value > result.value) {
            result.value = value;
            result.move = move;
        }

        alpha = std::max(alpha, result.value);

        if (result.value >= beta) {
            if (!(move & 4)) {  // quiet only - captures are ordered by SEE
                Killers.store(ply, move);  // killers are indexed by PLY (true, tracked separately from depth)
                History.update(Board.pit, move, depth);  // history by REMAINING depth
                for (int j = 0; j < i; ++j) // punish the quiets that failed before it
                    if (!(moves[j] & 4)) History.penalise(Board.pit, moves[j], depth);
                if (pp) {
                    Continuation.update(Board.pit, pp, pt,
                        Board.mailbox[_pext_u32(move, from_square_mask)], _pext_u32(move, to_square_mask), depth);
                    for (int j = 0; j < i; ++j)
                        if (!(moves[j] & 4))
                            Continuation.penalise(Board.pit, pp, pt,
                                Board.mailbox[_pext_u32(moves[j], from_square_mask)], _pext_u32(moves[j], to_square_mask), depth);
                }
            }
            break; // fail soft beta-cutoff
        }
    }

    Board.possible_moves = moves;   // restore the parent's move list (search's iterative-deepening loop relies on it)
    Board.possible_captures = captures;

    // TTEntry based on result.value and result.move after the loop completes
    if (!result.uci_stop) {
        char flag = result.value <= alphaOrig ? 'u' : (result.value >= beta ? 'l' : 'e');
        zobrist.store(parent_key, result.value, depth, result.move, flag);
    }
    
    return result;
}

best q_search (Chess & Board, int alpha, int beta, int depth, int & max_q_depth,std::chrono::steady_clock::time_point timeisup) {

    ++Stop.nodes;

    // No repetition check in q_search: it searches only captures/promotions (irreversible), so no
    // repetition is possible, and the q-root was already checked by alpha_beta. If check-evasions
    // (reversible king/block moves) are ever added here, RE-ADD the check + the push/pop below.
    
    uint64_t parent_key = Board.tpt.zobrist_key_64;   // save before any make() below mutates Board
    int alphaOrig = alpha;                             // for the store's bound flag below

    // TT probe. q-nodes are stored at depth 0, so there is no depth gate: any entry that matches the
    // key is usable here — a q-entry, or a deeper main-search entry that transposes into this position.
    tp_table* hit = zobrist.probe(parent_key);   // no depth gate: q-entries are depth 0, any key match is usable
    if (hit) {
        if (hit->flag == 'e') return {hit->value, hit->move};
        if (hit->flag == 'l' and hit->value >= beta)  return {hit->value, hit->move};
        if (hit->flag == 'u' and hit->value <= alpha) return {hit->value, hit->move};
    }
    
    if (Board.possible_moves.size() == 0) { // no legal moves - checkmate or stalemate
        if (Board.bitboard[status] & check_flag) return {-my_const::MATE + depth, 0}; //  mate
        else return {0, 0}; // stalemate = draw
    }
    
    if (Board.possible_captures.size() == 0) return {evaluation(Board), 0};
    best result {evaluation(Board), 0U};
    if (result.value >= beta) return result; // stand-pat fails high -> beta cutoff
    alpha = std::max(alpha, result.value); // raise alpha to the stand-pat floor
    
    int stand_pat = result.value; // static floor for delta pruning
    bool node_in_check = Board.bitboard[status] & check_flag; // never delta-prune while in check

    int good_caps = SEE.SEE_Sort(Board);

    MoveList captures = Board.possible_captures;   // copy - make() clears Board's list (now a stack copy)
    MovePicker qmp(false, good_caps,  0); // Construct the picker, passing the values already computed - quiet_start

    for (int i = 0; i < captures.size(); ++i) {
        uint16_t move = captures[i];
        if (qmp.stage(i,move)==MoveStage::LosingCapture and !(move&8)) continue;  // SEE prune: skip losing captures, exempt promotion
        // delta pruning (leaf cut): skip a capture that can't reach alpha even winning the target for free.
        // Exempt promotions (move&8) and never when in check; alpha rises as we find better moves.
        if (!node_in_check and !(move & 8)
            and stand_pat + my_const::piece_value[Board.mailbox[_pext_u32(move, to_square_mask)]] + Tune.delta_margin <= alpha)
            continue;
        // break and return last result at uci requested "stop" and at time budget constraint 
        if ((Stop.node_limit and Stop.nodes >= Stop.node_limit)
            or (((Stop.nodes & 2047) == 0) and (std::chrono::steady_clock::now() > timeisup))
            or Stop.stop.load(std::memory_order_relaxed)) {
            result.uci_stop = true;
            break;
        }

        // Make move (in place), recurse, then unmake to restore Board exactly
        Undo undo;
        make (Board, move, undo);
        zobrist.prefetch(Board.tpt.zobrist_key_64); // hide the child's TT miss under movegen
        generate_moves(Board);

        best valuation  = q_search (Board, -beta, -alpha, depth + 1, max_q_depth, timeisup);

        unmake (Board, move, undo);

        if (valuation.uci_stop) { result.uci_stop = true; break; }   // don't fold a half-searched value in

        int value = -valuation.value;
        max_q_depth = std::max(depth, max_q_depth);

        if (value > result.value) {
            result.value = value;
            alpha = std::max(alpha, value);
        }
        if (value >= beta) { // fail-soft beta-cutoff
            break;
        }
    }

    if (!result.uci_stop) {
        char flag = result.value <= alphaOrig ? 'u' : (result.value >= beta ? 'l' : 'e');
        zobrist.store(parent_key, result.value, 0, result.move, flag);   // q stores at depth 0
    }

    return result;
}