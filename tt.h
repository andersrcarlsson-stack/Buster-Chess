#include "global_header.h"      // <random> for mt19937_64
#include "global_constants.h"   

#ifndef TT_H
#define TT_H

class TranspositionTable {
private:
    std::mt19937_64 generator; // Repeatable seed
    std:: uniform_int_distribution<uint64_t> distribution;
    
public:
    TranspositionTable(uint64_t seed = 4294967291) : generator (seed), distribution(0, UINT64_MAX), transposition_table(entries) {}
    uint64_t random_number() {
        return distribution(generator);
    }
    
    uint64_t hash_piece[64][8][2];

    uint64_t entries    {1ULL << 23};  // 128 MiB of 16-byte entries — always a power of two
    uint64_t index_mask {(entries - 1) << 20};  // PEXT: key bits 20.. → slot. Declared ABOVE the vector:
                                                // members initialise in DECLARATION order, and the ctor
                                                // builds the vector from `entries`.
    std::vector<tp_table> transposition_table;

    Position_List position_list; // for detecting threefold repetition

    // format - Hash_piece[board square 0 - 63][piece type 2 - 7][color 0 - 1]
    // additional info tags for Castle Rights and e.p - Hash_piece[board pos a1,,h1,a8,,h8 and row 3 pos for white e.p and row 6 for black e.p][0 - 63][0][0]
    // additional info tags för Player In Turn (pit) - Hash_piece[0][0][1] - 1 is Black
    void generate_hash_pieces() {
        for (int i = 0; i < 64; ++i) {
            for (int j = 0; j < 8; ++j) { 
                for (int k = 0; k < 2; ++k) { // color 
                    hash_piece[i][j][k] = distribution(generator);
                }
            }
        }
        return;
    }

    void clear_table() {
        for (tp_table& e : transposition_table) e.zobrist_key_64 = 0ULL;
        position_list.clear();
    }

    // UCI `Hash`: MiB -> entries, rounded DOWN to a power of two (the slot index is a bit range of the
    // key, so the size must be 2^k). Frees the old table BEFORE allocating the new one, so peak memory
    // is one table, not two. Returns the MiB actually allocated. Caller must ensure no search is running.
    int resize(int mib) {
        mib = std::clamp(mib, 1, 4096); // >= 1 MiB keeps hashfull()'s 1000-slot sample valid
        uint64_t want = uint64_t(mib) * 1024 * 1024 / sizeof(tp_table);
        uint64_t n = 1;
        while (n * 2 <= want) n *= 2;  // largest power of two <= want
        std::vector<tp_table>().swap(transposition_table);  // release the old table first
        transposition_table.resize(n);  // value-initialised: every key 0 = empty
        entries    = n;
        index_mask = (n - 1) << 20;
        return int(n * sizeof(tp_table) >> 20);
    }


    // TT occupancy for UCI `info hashfull`, reported in PERMILL (0-1000).
    // Sampling trick: inspect only the FIRST 1000 slots and count the used ones.
    // The sample size IS 1000, so the count is already the permill - no division.
    // Fair sample: the slot index is PEXT of a uniformly random Zobrist key, so
    // entries land evenly across the table; slots 0..999 are as good as any 1000.
    // "Empty" == key 0, which is exactly what clear_table() writes.
    // Cost: ~1000 sequential reads (~24 KB) - and it is called ONCE PER ITERATIVE-
    // DEEPENING ITERATION, never per node, so it adds nothing to the hot path.
    // Hash is clamped to >= 1 MiB = 65536 entries, so the 1000-slot sample is always in range.
    int hashfull() {
        int used = 0;
        for (int i = 0; i < 1000; ++i)
            if (transposition_table[i].zobrist_key_64 != 0ULL) ++used;
        return used;
    }

    // --- TT access (one entry per slot) ---
    tp_table* probe(uint64_t key) {
        tp_table & slot = transposition_table[_pext_u64(key, index_mask)];
        return slot.zobrist_key_64 == key ? &slot : nullptr;   // hit -> pointer; miss -> nullptr
    }
    // Prefetch the slot a later probe(key) will read. A pure hint — no architectural effect,
    // so node counts MUST stay byte-identical. The table is 128 MiB against an 8 MiB L3, so a
    // probe is nearly always a DRAM miss (~200-300 cycles) paid at the child's first instruction.
    // Issued right after make(), the miss resolves underneath generate_moves instead of stalling.
    void prefetch(uint64_t key) const {
        __builtin_prefetch(&transposition_table[_pext_u64(key, index_mask)]);
    }
    void store(uint64_t key, int value, int depth, uint16_t move, char flag) {
        tp_table & slot = transposition_table[_pext_u64(key, index_mask)];
        // key-aware depth-preferred (same policy as #31): decline only a same-position shallower store
        if (slot.zobrist_key_64 != key or depth >= slot.depth) {
            slot.value = value; slot.depth = depth; slot.move = move;
            slot.flag = flag;   slot.zobrist_key_64 = key;
        }
    }

    void generate_initial_zobrist_key(Chess & Board) { 
        uint64_t traveling_one {1ULL}; 
        Board.tpt.zobrist_key_64 = 0ULL; // reset zobrist key

        for (int i = 0; i < 64; ++i) {
            if (Board.mailbox[i] !=0){
                Board.tpt.zobrist_key_64 ^= hash_piece[i][Board.mailbox[i]][!!(Board.bitboard[Black] & traveling_one)]; // if Black is 1 - White is 0 - only one required
            }
            if (Board.bitboard[status] & ep_castling_mask & traveling_one) { // castling rights plus ep squares
                Board.tpt.zobrist_key_64 ^= hash_piece[i][0][0];
            }
            traveling_one <<= 1;
        }
        if (Board.pit) {
            Board.tpt.zobrist_key_64 ^= hash_piece[0][0][1]; // PIT = 1 - Black to move
        }
    }
};
extern TranspositionTable zobrist;   // declaration, no initialiser, name = zobrist

#endif