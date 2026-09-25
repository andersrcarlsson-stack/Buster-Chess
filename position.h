#include "support_files.h"

#ifndef POSITION_H
#define POSITION_H  

void make_new_state (Chess &, uint16_t);
void make (Chess &, uint16_t, Undo &);            // in-place make (applies the move with make_new_state)
void unmake (Chess &, uint16_t, const Undo &);    // reverse the move using the Undo ticket
void make_null (Chess &, Undo &);                 // null move (pass): flip stm + clear e.p., key-consistent
void unmake_null (Chess &, const Undo &);  

#endif