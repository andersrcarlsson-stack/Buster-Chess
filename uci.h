#include "global_header.h"          // std::thread, string, vector, cstdint

#ifndef UCI_H
#define UCI_H
class GUI_Interface {
    std::thread ghost; // private data member — STAYS here (part of the type)
public:
    uint16_t decode_cli_move(const Chess & Board, std::string move);
    std::string decode_buster_move(uint16_t move);
    std::string decode_buster_to_algebraic(Chess & Board, uint16_t & move);
    bool handleUCI();
    void handleSetOption(const std::vector<std::string>& command);
    void handleStop();
    void stop_search();
    void handleIsReady();
    Chess handlePosition(std::vector<std::string> command);
    void handleGo(const Chess Board, std::vector<std::string> command);
    uint64_t key_after(const std::vector<std::string> & moves);
    int halfmove_after(const std::vector<std::string> & moves);
    void handleTest(const Chess Board, std::vector<std::string> command);
    void handleEval(const Chess Board);
    void handlePerft(const Chess Board, std::vector<std::string> command);
    void game_loop(Chess Board);
};
extern GUI_Interface uci;          // name-match, no initialiser

void run_uci(void); 

#endif