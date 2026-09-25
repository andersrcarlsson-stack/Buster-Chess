#include "global_header.h" // std::string, std::cout, int long long

#ifndef TESTHARNESS_H
#define TESTHARNESS_H
class TestHarness {
    int run = 0, failed = 0;
public:
    void check(bool cond, const std::string & name) {
        ++run;
        if (!cond and ++failed <= 10) std::cout << "FAIL: " << name << "\n";
    }
    void reset()   { run = 0; failed = 0; }
    void summary() { std::cout << "tests: " << (run - failed) << "/" << run << " passed\n"; }
    int  failures() const { return failed; }
};
Chess mirror_board(const Chess & b);
void  verify_walk(const Chess & Board, int depth, TestHarness & tests);
int long long perft (const Chess, int);

#endif