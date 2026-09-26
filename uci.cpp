#include "global_header.h"
#include "support_files.h"   // Chess, MoveList, fen_input, search, perft, evaluation, make_new_state, generate_moves, make, unmake
#include "global_constants.h"// from/to_square_mask etc. in the decode helpers
#include "piece_table.h"
#include "search_state.h"    // Stop, Tune, Killers, History
#include "tt.h"              // zobrist
#include "tables.h"          // Lookup
#include "testharness.h"     // TestHarness, mirror_board, verify_walk
#include "uci.h"             // GUI_Interface + extern uci
#include "movegen.h"
#include "position.h" // make_new_state, make, unmake, make_null, unmake_null
#include "search.h"
#include "eval.h"
#include "fen_input.h"

GUI_Interface uci; // THE single definition

uint16_t GUI_Interface::decode_cli_move (const Chess & Board, std::string move) {

    uint16_t buster_move {0};
    unsigned flag {0};
    bool promotion_capture {false};

    std::string from = move.substr(0, 2);
    std::string to = move.substr(2, 2);

    uint16_t from_square = std::distance(my_const::square_name.begin(), find(my_const::square_name.begin(), my_const::square_name.end(), from));
    uint16_t to_square = std::distance(my_const::square_name.begin(), find(my_const::square_name.begin(), my_const::square_name.end(), to));

    buster_move = (from_square << 10 | to_square << 4);

    if (move.size() == 5) { // takes care of 8 promotion flag settings (8 - 15)
        promotion_capture = !(abs(from_square - to_square) == 8);
        const char promotion_flag = move[4];

        switch (promotion_flag) {
        case 'n':
            flag = 8 + 4 * promotion_capture;
            break;
        case 'b':
            flag = 9 + 4 * promotion_capture;
            break;
        case 'r':
            flag = 10 + 4 * promotion_capture;
            break;
        case 'q':
            flag = 11 + 4 * promotion_capture;
            break;
        default:
            break;
        }
        
        return buster_move + flag;
    }

    if (Board.mailbox[from_square] == Pawn and ((from_square > 7 and from_square < 16) or (from_square > 47 and from_square < 56)) and 
    ((to_square > 23 and to_square < 32) or (to_square > 31 and to_square < 40))) { 
        return buster_move + 1; // double pawn push
    }
    if (Board.mailbox[from_square] == King and (from_square == 4 or from_square == 60) and (to_square == 6 or to_square == 62)) {
        return buster_move + 2;  // king side castle
    }
    if (Board.mailbox[from_square] == King and (from_square == 4 or from_square == 60) and (to_square == 2 or to_square == 58)) { 
        return buster_move + 3; // queen side castle
    }
    if (Board.mailbox[to_square] != 0) { 
        return buster_move + 4; // regular capture
    }
    if (Board.mailbox[from_square] == Pawn and ((1ULL << to_square) & (Board.bitboard[status] & (my_const::row_3_mask | my_const::row_6_mask)))) { 
        return buster_move + 5; // ep capture
    }
    if (Board.mailbox[to_square] == 0 and !(Board.mailbox[from_square] == Pawn and (Board.bitboard[status] & ((1ULL << to_square) & (my_const::row_3_mask | my_const::row_6_mask))))) { // regular quiet move - no ep !
    return buster_move; // the flag is 0 - no calculation needed !
    }

    return buster_move;
}

std::string GUI_Interface::decode_buster_move(uint16_t move){
    std::string promotion_flag;
    uint16_t from = (move & from_square_mask) >> 10;
    uint16_t to = (move & to_square_mask) >> 4;
    if ((move & 15) == 8 or (move & 15) == 12) promotion_flag = "n";
    else if ((move & 15) == 9 or (move & 15) == 13) promotion_flag = "b";
    else if ((move & 15) == 10 or (move & 15) == 14) promotion_flag = "r";
    else if ((move & 15) == 11 or (move & 15) == 15) promotion_flag = "q";
    else promotion_flag = "";

    return std::string(my_const::square_name[from]) + std::string(my_const::square_name[to]) + promotion_flag;
}

std::string GUI_Interface::decode_buster_to_algebraic(Chess & Board, uint16_t & move) {
    bool same_file {false}, same_rank {false};
    int other_count {0};
    std::string extra_info = "";
    int from = _pext_u32(move, from_square_mask);
    int to = _pext_u32(move, to_square_mask);
    int piece = Board.mailbox[from];
    std::string_view alg_from = algebra_pieces[piece]; // pawn is coded with "" empty string...

    if ((move & 15) == 2) return std::string ("O-O");     // flag 2 = king side (movegen: e1g1, make: case 2)
    if ((move & 15) == 3) return std::string ("O-O-O");   // flag 3 = queen side

    for (int i = 0; i < Board.possible_moves.size(); ++i) { // Disambiguating moves check
        uint16_t cand = Board.possible_moves[i];
        if (int(_pext_u32(cand, to_square_mask)) != to) continue; // must aim at the same square
        int cand_from = _pext_u32(cand, from_square_mask);
        if (cand_from == from) continue;   // self OR a sibling promotion (=R/=B/=N) — same piece, not a rival
        if (Board.mailbox[cand_from] != piece) continue;  // must be the same piece type
        // a genuine rival for this square:
            ++ other_count;
        if (cand_from % 8 == from % 8) same_file = true;
        if (cand_from / 8 == from / 8) same_rank = true;
    }

    if (other_count > 0) {
        if (!same_file) extra_info = std::string(1, 'a' + (from % 8));        // file differs → file letter
        else if (!same_rank) extra_info = std::string(1, '1' + (from / 8));   // file shared → rank digit
        else  extra_info = std::string(1, 'a' + (from % 8))  + std::string(1, '1' + (from / 8));      // both shared → full square  
    }

    std::string promo = "";
    if (move & 8) promo = "=" + std::string(algebra_pieces[(move & 3) + Knight]);

    bool capture = move & 4;                        // bit 2 = capture (incl. ep + capture-promos)
    if (piece == Pawn and capture)  extra_info = std::string(1, 'a' + (from % 8));   // pawn captures

    return std::string(alg_from) + extra_info + (capture ? "x" : "") + std::string(my_const::square_name[to]) + promo;

}

bool GUI_Interface::handleUCI() {
    bool uci_set = true;
    std::cout << "id name Buster " << engine_version << std::endl;
    std::cout << "id author Anders R Carlsson" << std::endl;
    std::cout << "option name Hash type spin default 128 min 1 max 4096" << std::endl;
    std::cout << "option name FutilityMargin type spin default 106 min 0 max 500" << std::endl;
    std::cout << "option name FutilityDepth type spin default 5 min 0 max 8" << std::endl;
    std::cout << "option name LmrReduction type spin default 2 min 0 max 3" << std::endl;
    std::cout << "option name NullMoveR type spin default 2 min 1 max 4" << std::endl;
    std::cout << "option name CheckExt type spin default 1 min 0 max 1" << std::endl;
    std::cout << "option name RfpMargin type spin default 70 min 0 max 500" << std::endl;
    std::cout << "option name LmpDepth type spin default 3 min 0 max 8" << std::endl;
    std::cout << "option name AspDelta type spin default 16 min 5 max 200" << std::endl;
    std::cout << "option name DeltaMargin type spin default 190 min 0 max 500" << std::endl;
    std::cout << "option name RfpDepth type spin default 8 min 0 max 10" << std::endl;
    std::cout << "option name AspMinDepth type spin default 3 min 2 max 8" << std::endl;

    std::cout << "uciok" << std::endl;
    return uci_set;
}

void GUI_Interface::handleSetOption(const std::vector<std::string>& command) {
if (command.size() < 5 || command[1] != "name" || command[3] != "value") return;
const std::string& name = command[2];
auto parsed = parse_int(command[4]);
if (!parsed) { std::cout << "info string ignoring option " << name << ": value is not a whole number\n"; return; }
int value = *parsed;
if (name == "Hash") {
    std::cout << "info string Hash " << zobrist.resize(value) << " MiB\n";   // actual, after rounding
    return;
}
if      (name == "FutilityMargin") Tune.futility_margin = value;
else if (name == "FutilityDepth")  Tune.futility_depth  = value;
else if (name == "LmrReduction") Tune.lmr_reduction = value;
else if (name == "NullMoveR")    Tune.null_move_r   = value;
else if (name == "CheckExt")     Tune.check_ext     = value;
else if (name == "RfpMargin")    Tune.rfp_margin    = value;
else if (name == "LmpDepth")     Tune.lmp_depth     = value;
else if (name == "AspDelta")     Tune.asp_delta     = value;
else if (name == "DeltaMargin")  Tune.delta_margin  = value;
else if (name == "RfpDepth")     Tune.rfp_depth     = value;
else if (name == "AspMinDepth")  Tune.asp_min_depth = value;
else { std::cout << "info string unknown option " << name << "\n"; return; }
std::cout << "info string set " << name << " = " << value << "\n";
}

void GUI_Interface::handleStop() { // only started.... at "start" load an atomic bool for stop.....
    Stop.stop.store(true, std::memory_order_relaxed);
    return ;
}

// Stop any running search and wait for its thread. Every command that touches state the search
// thread shares (position_list, TT, History/Killers/Continuation, Tune, Stop) calls this first.
// Returns at once when nothing is running - a finished thread joins instantly.
void GUI_Interface::stop_search() {
    handleStop();
    if (ghost.joinable()) ghost.join();
}

void GUI_Interface::handleIsReady() {
std::cout << "readyok" << std::endl;
}

Chess GUI_Interface::handlePosition(std::vector<std::string> command) {
    Chess Board {};
    uint16_t buster_move;
    std::string move;
    bool initial_pos_set {false};
    zobrist.position_list.clear();

    if (command[1] == "fen") {
        if (command.size() >= 8) {                     // enough fields to index [2..7]?
            Board = fen_input(command[2], command[3], command[4], command[5], command[6], command[7]);
            if (Board.bitboard[King] != 0) initial_pos_set = true;   // your king-check
        }
        // else: too few fields -> malformed -> leave initial_pos_set false
    }
    else if (command[1] == "startpos") {
        Board = fen_input("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
        initial_pos_set = true; // initial pos is set with "startpos"... OK to add a move if desired
    }
    else if (command[1] == "kiwipete") {
        Board = fen_input("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "1");
        initial_pos_set = true; // initial pos is set with "startpos"... OK to add a move if desired
    }

    zobrist.generate_initial_zobrist_key(Board); // at first position - initiate zobrist key for state

    if ((std::find(command.begin(), command.end(), "moves") != command.end()) and initial_pos_set) { // command string includes more than startpos - one move is sure to be there
        auto it = std::find(command.begin(), command.end(), "moves");
        int index = std::distance(command.begin(), it);
        for (size_t i = index + 1; i < command.size(); ++i) { // send all the decoded moves to make_new_state
            buster_move = decode_cli_move (Board, command[i]);
            make_new_state(Board, buster_move);                // repetition handlling
            if (!(Board.bitboard[status] & halfmove_clock_mask)) zobrist.position_list.clear(); // clear position list if halfmove clock has been cleared
            zobrist.position_list.push_back(Board.tpt.zobrist_key_64); // add the new position to the repetition list
        }
    }

    if (initial_pos_set) generate_moves(Board); 
    
    return Board;
}

void GUI_Interface::handleGo(const Chess Board, std::vector<std::string> command) {

    if (Board.possible_moves.size() == 0) {
        std::cout << "bestmove " << "0000" << std::endl; // checkmate / stalemate
        return;
    }

    // helper: the integer after <key>, or <fallback> if the key (or its value) is absent
    auto get_int = [&command](const std::string& key, int fallback) {
        auto it = std::find(command.begin(), command.end(), key);
        if (it == command.end() or it + 1 == command.end()) return fallback;
        auto v = parse_int(*(it + 1));
        if (!v) std::cout << "info string ignoring " << key << " " << *(it + 1) << ": not a whole number\n";
        return v.value_or(fallback);
    };

    int depth     = get_int("depth", 40);      // fixed depth if asked; otherwise the iterative-deepening ceiling
    int movetime  = get_int("movetime", 0);
    int wtime     = get_int("wtime", 0);
    int btime     = get_int("btime", 0);
    int winc      = get_int("winc", 0);
    int binc      = get_int("binc", 0);
    int movestogo = get_int("movestogo", 0);   // 0 ⇒ sudden death (no time control boundary)
    int nodes = get_int("nodes", 0);

    int my_time = Board.pit ? btime : wtime;   // pit: 0 = White to move, 1 = Black
    int my_inc  = Board.pit ? binc  : winc;

    int soft, hard;                                         // soft: don't START an iteration past this
    if (movetime > 0) soft = hard = movetime;               // search exactly this long
    else if (my_time > 0) {                                 // budget from the clock
        int target = my_time / (movestogo ? std::min(movestogo, 20) : 20) + my_inc * 3 / 4;
        hard = std::min(3 * target, my_time / 2);           // abort: never bet more than half the clock
        soft = std::min(target / 2, hard);                  // next iteration costs ~0.8x all spent so far
        hard = std::max(hard, 1);
        soft = std::max(soft, 1);
    }
    else soft = hard = 9999999;                             // no limits -> analyse until 'stop'

    stop_search();
    Stop.node_limit = nodes;      // 0 ⇒ unlimited
    Stop.nodes = 0;          // fresh count for this 'go'
    Stop.stop.store(false, std::memory_order_relaxed);       // re-arm the stop flag
    ghost = std::thread(search, Board, depth, soft, hard);
}

uint64_t GUI_Interface::key_after(const std::vector<std::string> & moves) {
    Chess b = fen_input("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
    zobrist.generate_initial_zobrist_key(b);                 // set the starting key
    for (const std::string & mv : moves)
    make_new_state(b, decode_cli_move(b, mv));    
    return b.tpt.zobrist_key_64;
}

int GUI_Interface::halfmove_after(const std::vector<std::string> & moves) {
    Chess b = fen_input("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
    for (const std::string & mv : moves) make_new_state(b, decode_cli_move(b, mv));
    return _pext_u64(b.bitboard[status], halfmove_clock_mask);   // read bits 25-31 back
}

void GUI_Interface::handleTest(const Chess Board, std::vector<std::string> command) {
    TestHarness tests;                           // local — lifetime = this call

    tests.check(1 + 1 == 2, "harness alive");

    Chess copy = Board;
    uint64_t before = copy.tpt.zobrist_key_64;
    zobrist.generate_initial_zobrist_key(copy);
    tests.check(copy.tpt.zobrist_key_64 == before, "root key recompute == stored");

    // transposition equality: same position reached two ways → identical key
    std::vector<std::string> path_a {"g1f3", "b8c6", "b1c3", "g8f6"};
    std::vector<std::string> path_b {"b1c3", "g8f6", "g1f3", "b8c6"};
    tests.check(key_after(path_a) == key_after(path_b), "transposition equality (knights)");

    // castling-right change must also be path-independent
    std::vector<std::string> castle_a {"b1c3", "b8c6", "a1b1", "g8f6", "b1a1"};
    std::vector<std::string> castle_b {"b1c3", "g8f6", "a1b1", "b8c6", "b1a1"};
    tests.check(key_after(castle_a) == key_after(castle_b), "transposition equality (Q-side right lost)");
    // knight promotions must decode with the 'n' suffix (flag 8 = quiet, 12 = capture)
    uint16_t np_quiet   = (52 << 10) | (60 << 4) | 8;   // e7-e8=N
    uint16_t np_capture = (52 << 10) | (61 << 4) | 12;  // e7xf8=N
    tests.check(decode_buster_move(np_quiet).back()   == 'n', "knight promo (quiet)  -> ...n");
    tests.check(decode_buster_move(np_capture).back() == 'n', "knight promo (capture) -> ...n");
    // contrast: a queen promo should end in 'q', proving we're testing the suffix, not luck
    uint16_t qp_quiet   = (52 << 10) | (60 << 4) | 11;  // e7-e8=Q
    tests.check(decode_buster_move(qp_quiet).back()    == 'q', "     queen promo (quiet)  -> ...q");
    // castling SAN: flag 2 is king side, flag 3 queen side — both decoders must agree with movegen/make
    Chess cb = fen_input("r3k2r/8/8/8/8/8/8/R3K2R", "w", "KQkq", "-", "0", "1");
    uint16_t oo  = (4 << 10) | (6 << 4) | 2;   // e1g1
    uint16_t ooo = (4 << 10) | (2 << 4) | 3;   // e1c1
    tests.check(decode_buster_to_algebraic(cb, oo)  == "O-O",   "SAN: king-side castling prints O-O");
    tests.check(decode_buster_to_algebraic(cb, ooo) == "O-O-O", "SAN: queen-side castling prints O-O-O");
    // e.p. capture onto a HIGH square (row 6, index >= 32) must decode as ep (flag 5).
    // (A 32-bit `1 << to_square` would be undefined behaviour for to_square >= 32.)
    Chess ep_board = fen_input("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR",
                            "w", "-", "d6", "0", "1");
    uint16_t ep_move = decode_cli_move(ep_board, "e5d6");   // white e5 x d6 e.p.
    tests.check((ep_move & 15) == 5, "high-square e.p. capture decodes as ep (flag 5)");

    // halfmove clock increments on quiet non-pawn moves, resets on pawn moves/captures
    tests.check(halfmove_after({"g1f3"})                         == 1, "quiet move increments clock");
    tests.check(halfmove_after({"g1f3","b8c6","b1c3","g8f6"})    == 4, "clock counts up over quiet moves");
    tests.check(halfmove_after({"g1f3","b8c6","e2e4"})           == 0, "pawn move resets clock");

    // perft as regression: known FEN -> known node count.
    // (perft(Board, n) = perft to depth n; perft also prints a per-root "divide" breakdown,
    //  so we silence cout around the calls and check the RETURNED counts.)
    Chess sp = fen_input("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
    generate_moves(sp);
    Chess kp = fen_input("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "1");
    generate_moves(kp);
    std::ostringstream sink;
    std::streambuf * old_buf = std::cout.rdbuf(sink.rdbuf());   // redirect cout -> sink
    int long long sp_nodes = perft(sp, 4);
    int long long kp_nodes = perft(kp, 3);
    std::cout.rdbuf(old_buf);                                   // restore cout

    tests.check(sp_nodes == 197281, "perft regression: startpos depth 4 = 197281");
    tests.check(kp_nodes == 97862,  "perft regression: kiwipete depth 3 = 97862");

    // e.p. coverage: two identical boards differing ONLY in the e.p. right must hash differently.
    // (proves the e.p. target bit actually participates in the key.)
    Chess with_ep = fen_input("rnbqkbnr/pppp1ppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", "b", "KQkq", "e3", "0", "1");
    Chess no_ep   = fen_input("rnbqkbnr/pppp1ppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", "b", "KQkq", "-",  "0", "1");
    zobrist.generate_initial_zobrist_key(with_ep);
    zobrist.generate_initial_zobrist_key(no_ep);
    tests.check(with_ep.tpt.zobrist_key_64 != no_ep.tpt.zobrist_key_64, "e.p. right changes the key");

    // eval symmetry — three layers, because they catch different bug classes:
    //
    // (1) EXHAUSTIVE TABLE CHECK — the real guard. Every colour must mirror the other, in BOTH
    //     phases: pst[phase][piece][White][sq] == pst[phase][piece][Black][sq ^ 56]. Phase-
    //     independent, so it catches an asymmetric ENDGAME entry that any full-material test
    //     position would weight to ~0 and miss (a dropped comma, a mistyped mirror).
    bool pst_symmetric = true;
    for (int ph = 0; ph < 2; ++ph)
        for (int pc = 0; pc < 6; ++pc)
            for (int sq = 0; sq < 64; ++sq)
                if (piece_square_tables[ph][pc][White][sq] != piece_square_tables[ph][pc][Black][sq ^ 56])
                    pst_symmetric = false;
    tests.check(pst_symmetric, "eval: piece-square tables colour-symmetric (both phases)");

    // (1b) EXHAUSTIVE king-zone symmetry — the king_safety[colour][sq] bitboards are hand-typed,
    //      so verify White mirrors Black: king_safety[0][sq] == bswap(king_safety[1][sq ^ 56]).
    //      sq ^ 56 mirrors the rank; __builtin_bswap64 mirrors the bitboard. Catches a transcription
    //      slip (a dropped/duplicated square) that the per-position mirror test would almost always miss.
    bool king_zone_symmetric = true;
    for (int sq = 0; sq < 64; ++sq)
        if (my_const::king_safety[White][sq] != __builtin_bswap64(my_const::king_safety[Black][sq ^ 56]))
            king_zone_symmetric = false;
    tests.check(king_zone_symmetric, "eval: king-safety zones colour-symmetric");

    // (1c) passed-pawn masks — generated symmetrically, but guard against a builder edit
    bool passed_symmetric = true;
    for (int sq = 0; sq < 64; ++sq)
        if (Lookup.passed_pawn[White][sq] != __builtin_bswap64(Lookup.passed_pawn[Black][sq ^ 56]))
            passed_symmetric = false;
    tests.check(passed_symmetric, "eval: passed-pawn masks colour-symmetric");

    // (2) a colour-symmetric position must score EXACTLY 0 (tempo-free eval).
    Chess sym = fen_input("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
    tests.check(evaluation(sym) == 15, "eval: symmetric start position == 0");

    // (3) mirror invariance of the eval FUNCTION (phase blend + signs), in both a full-material and
    //     a bare endgame position so the mg and eg blend weights each get exercised.
    Chess asym = fen_input("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "1");
    tests.check(evaluation(asym) == evaluation(mirror_board(asym)), "eval: mirror invariance (midgame)");
    Chess eg = fen_input("8/2k5/5p2/1P6/8/4P3/5K2/8", "w", "-", "-", "0", "1");
    tests.check(evaluation(eg) == evaluation(mirror_board(eg)), "eval: mirror invariance (endgame)");

    // null-move primitive on a position WITH an e.p. right (guarantees the e.p.-clearing path runs):
    // after a pass the key must match a from-scratch recompute, and make/unmake must round-trip.
    Chess np = fen_input("rnbqkbnr/pppp1ppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR", "b", "KQkq", "e3", "0", "1");
    zobrist.generate_initial_zobrist_key(np);
    Chess np_orig = np;                              // snapshot for the round-trip
    Undo nu;
    make_null(np, nu);
    Chess np_rc = np; zobrist.generate_initial_zobrist_key(np_rc);
    tests.check(np.tpt.zobrist_key_64 == np_rc.tpt.zobrist_key_64, "null-move: key consistent after pass (e.p. cleared)");
    unmake_null(np, nu);
    tests.check(np.bitboard == np_orig.bitboard and np.mailbox == np_orig.mailbox and np.pit == np_orig.pit
                and np.tpt.zobrist_key_64 == np_orig.tpt.zobrist_key_64, "null-move: round-trip restores board");

    int depth = 4;
    if (command.size() > 1) depth = parse_int(command[1]).value_or(depth);   // "test 5" → depth 5
    verify_walk(Board, depth, tests);

    tests.summary();
}

// Debug-only STATIC eval readout. NOT in the hot path — this is the UCI command layer, which
// the search never enters, so there is nothing here to measure or to regress.
//
// `go depth 1` is NOT a static eval: it returns the score of the best CHILD, so the root
// evaluation is never visible. This command shows it directly.
//
// The mirror line is here rather than in a separate command so that the check you want after
// EVERY eval edit is one keystroke. Note the invariant is e == em, NOT e == -em: mirror_board
// swaps the colours AND the side to move, so a correct eval returns the SAME number from the
// mirrored side's point of view.
void GUI_Interface::handleEval(const Chess Board) {
    Chess m = mirror_board(Board);
    int e  = evaluation(Board);
    int em = evaluation(m);
    std::cout << "eval " << e << " (cp, side-to-move POV)" << std::endl;
    std::cout << "mirror " << em << "  diff " << (e - em)
              << (e == em ? "  OK" : "  <-- ASYMMETRIC: eval bug") << std::endl;
}

void GUI_Interface::handlePerft(const Chess Board, std::vector<std::string> command) {

    auto it = std::find(command.begin(), command.end(), "depth");
    auto d  = (it != command.end() and it + 1 != command.end()) ? parse_int(*(it + 1)) : std::nullopt;
    if (!d or *d < 1) { std::cout << "info string usage: perft depth <n>, n >= 1\n"; return; }
    int depth = *d;

    // clock_t start = clock();
    auto start = std::chrono::steady_clock::now();
    int long long perft_output = perft (Board, depth);
    // clock_t end = clock();
    // double elapsed = double(end - start)/CLOCKS_PER_SEC;
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "The number of nodes found: " << perft_output << " \n";
    std::cout << "Time used for generation; " << elapsed.count() << "  milliseconds" << " \n" << " \n";
}

void GUI_Interface::game_loop (Chess Board) {

    std::string from_gui, temp; 
    bool game_ongoing {false};
    Stop.stop.store(false, std::memory_order_relaxed);

    while (true) {

        if (!std::getline(std::cin, from_gui)) break;   // EOF: the GUI is gone, exit rather than spin

        // parse input string from GUI and clean up according to UCI Protocol
        // Tokenizing by space ' ' as separating char
        std::vector <std::string> tokens, command; 
        std::stringstream check(from_gui);
        while(getline(check, temp, ' ')) {
            if (temp != "") tokens.push_back(temp);
        }

        int uci_index {99}; // rinse original input string from white space and garbage starts - assign uci_index to first actual command in input string
        for (std::string t : tokens) {
            auto first_valid_command = find(my_const::uci_commands.begin(), my_const::uci_commands.end(), t);
            if (first_valid_command != my_const::uci_commands.end()) {
                uci_index = distance(my_const::uci_commands.begin(), first_valid_command);
                break; 
            }
        }

        if (uci_index == 99) continue; // if no valid uci commands found in input string - start over from new input
        auto valid_command = find(tokens.begin(), tokens.end(), my_const::uci_commands[uci_index]);
        command.assign (valid_command, tokens.end()); // keep everything intact after the valid uci command and process that ....
        // std::vector command is now clean - cli command in position [0] - tokens as from original input string follow in rest of vector
        if (command[0] == "uci") {
            handleUCI();
        } else if (command[0] == "isready") {
            handleIsReady();
        } else if (command[0] == "position") {
            stop_search();
            Chess candidate = handlePosition(command);
            if (candidate.bitboard[King] != 0) {      // only accept a real position
                Board = candidate;
                game_ongoing = true;
            } else {
                std::cout << "info string ignoring malformed FEN\n";   // keep the previous Board
            }
        } else if (command[0] == "go" and game_ongoing) { 
            handleGo(Board, command);
            game_ongoing = false;
        } else if (command[0] == "perft") {
            handlePerft(Board, command);
        } else if (command[0] == "test") {
            handleTest(Board, command);
        } else if (command[0] == "eval") {
            handleEval(Board);
        } else if (command[0] == "stop") { // started work on stop - handle stop will load an atomic bool
            handleStop();
        } else if (command[0] == "ucinewgame") {
            stop_search();
            zobrist.clear_table();
            Killers.clear();
            History.clear();
            Continuation.clear();
        } else if (command[0] == "setoption") {
            stop_search();
            handleSetOption(command);
        } else if (command[0] == "quit") {
            break;            
        }
    }
    // Single shutdown for EVERY way out of the loop (quit, stdin EOF, any future break).
    // `ghost` lives in the global `uci`; if it is still joinable when static destruction reaches it,
    // std::terminate fires - and by then the globals the search reads may already be gone.
    stop_search();                         // join BEFORE main returns, while every global is alive

}

void run_uci(void) {
    Chess Board {};
    zobrist.generate_hash_pieces();
    uci.game_loop(Board);
}