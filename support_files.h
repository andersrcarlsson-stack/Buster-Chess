#ifndef BINARY_PERMUT_H
#define BINARY_PERMUT_H
struct two_vectors {std::vector<uint64_t> mask; std::vector<std::vector<uint64_t>> lookup;};
template <std::size_t N>
struct flat_slider {
    std::array<uint64_t, N>   data;    // all 64 sub-tables concatenated, contiguous
    std::array<int, 64>       offset;  // start index of square sq inside data
    std::array<uint64_t, 64>  mask;    // per-square PEXT mask

    // the whole indirection lives here, in ONE place:
    uint64_t attacks (int sq, uint64_t occ) const {
        return data[ offset[sq] + _pext_u64(occ, mask[sq]) ];
    }
};

template <std::size_t N>
flat_slider<N> flatten (const two_vectors &tv) {
    flat_slider<N> f;
    int running = 0;
    for (int sq = 0; sq < 64; ++sq) {
        f.offset[sq] = running;
        f.mask[sq]   = tv.mask[sq];
        for (uint64_t entry : tv.lookup[sq])
            f.data[running++] = entry;
    }
    assert (running == static_cast<int>(N));   // built size must equal declared N
    return f;
}

struct tp_table {uint64_t zobrist_key_64; int16_t value; int16_t depth; uint16_t move; char flag; uint8_t generation;}; // 16 bytes, four per cache line; generation is reserved (unused)
// Fixed-size move list (max legal moves ~218) - lives on the stack, no per-node heap alloc.
struct MoveList {
    std::array<uint16_t, 256> data;
    int count = 0;
    MoveList() = default;
    MoveList(const MoveList& o) : count(o.count)
        { std::copy_n(o.data.begin(), o.count, data.begin()); }
    MoveList& operator=(const MoveList& o) {
        count = o.count;
        std::copy_n(o.data.begin(), o.count, data.begin());
        return *this;
    }

    void push_back(uint16_t m) { data[count++] = m; }
    void clear() { count = 0; }
    int  size()  const { return count; }
    bool empty() const { return count == 0; }
    uint16_t&       operator[](int i)       { return data[i]; }
    const uint16_t& operator[](int i) const { return data[i]; }
    uint16_t* begin() { return data.data(); }
    uint16_t* end()   { return data.data() + count; }
    const uint16_t* begin() const { return data.data(); }
    const uint16_t* end()   const { return data.data() + count; }
    uint16_t front() const { return count ? data[0] : 0; }   // empty list -> 0 (move "none"), never an uninitialised read
};

// Repetition key stack — game history + current search path, merged, for in-search
// repetition detection. Fixed-capacity, heap-free (mirrors MoveList, but holds 64-bit
// Zobrist keys and supports pop_back for the unmake side of the search loop).
struct Position_List {
    static constexpr int capacity = 512;   // game history (~100) + search depth (~55) + quiescence (~40), with margin
    std::array<uint64_t, capacity> data;
    int count = 0;

    void push_back(uint64_t key) { assert(count < capacity); data[count++] = key; }
    void pop_back()              { assert(count > 0); --count; }   // peel one search-path key (in unmake)
    void clear()                 { count = 0; }
    int  size()  const           { return count; }
    bool empty() const           { return count == 0; }
    uint64_t  back() const                  { return data[count - 1]; }
    uint64_t& operator[](int i)             { return data[i]; }
    const uint64_t& operator[](int i) const { return data[i]; }
    uint64_t*       begin()       { return data.data(); }
    uint64_t*       end()         { return data.data() + count; }
    const uint64_t* begin() const { return data.data(); }
    const uint64_t* end()   const { return data.data() + count; }
};

struct Chess {std::array<uint64_t, 9> bitboard; bool pit; std::array<uint16_t, 64> mailbox; MoveList possible_moves; MoveList possible_captures; tp_table tpt;};
struct Undo {uint16_t captured; uint64_t old_status; uint64_t old_key;}; // make/unmake ticket: the info a move destroys
struct best {int value; uint16_t move; bool uci_stop {false};};

// Parse a WHOLE token as an int: nullopt for "", "abc", "12abc" or out of range. Never throws.
inline std::optional<int> parse_int(std::string_view s) {
    int value = 0;
    auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() or end != s.data() + s.size()) return std::nullopt;
    return value;
}

#endif

// Chess: bitboard[9], pit (side to move: 0 White, 1 Black), mailbox[64] (piece type per square,
// 0 = empty), the legal move lists, and tpt (carries the Zobrist key). bitboard[] layout:
// [0] White pieces   [1] Black pieces
// [2] Pawns  [3] Knights  [4] Bishops  [5] Rooks  [6] Queens  [7] Kings   (both colours)
// [8] the status word:
//     bits 0, 7, 56, 63   castling rights, on the rook's home square (a1, h1, a8, h8)
//     bits 16-23, 40-47   en-passant target squares (ranks 3 and 6)
//     bit 24              side to move is in check (set by generate_moves)
//     bits 25-31          fifty-move (half-move) counter
// Moves are 16 bits: from-square (bits 10-15), to-square (bits 4-9), flag (bits 0-3).
// See docs/ARCHITECTURE.md for the flag values.
