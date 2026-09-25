#include "support_files.h"
#include "global_header.h"

#ifndef SEARCH_H
#define SEARCH_H

void search (Chess, int, int, int);
best alpha_beta (Chess &, int, int, int, int, int &, std::chrono::steady_clock::time_point, bool can_null = true);
best q_search (Chess &, int, int, int, int &, std::chrono::steady_clock::time_point);

#endif