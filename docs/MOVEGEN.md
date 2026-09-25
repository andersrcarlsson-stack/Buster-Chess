# Move generation

How Buster 7.0 attacks, generates legal moves, applies and reverts them, hashes positions and
recognises repetitions. Data layouts (bitboards, the status word, the 16-bit move) are in
[ARCHITECTURE.md](ARCHITECTURE.md).

## Principles

- **Fully legal generation.** Every move `generate_moves` produces is legal. Pins, checks, the
  king's own safety and the en-passant special cases are all resolved during generation, so the
  search never makes a move only to discover it was illegal.
- **One pass per position.** A call fills two lists on the board: `possible_captures` (captures,
  including en passant and capturing promotions) and `possible_moves` (**all** legal moves:
  the captures first, then the quiet moves). It also sets or clears the *in check* bit of the
  status word.
- **The king is never a capture target.** The enemy king is removed from every target set, so a
  generated capture of the king cannot exist.

## Slider attacks through `PEXT`

### The lookup

Bishop and rook attacks come from two precomputed tables, looked up with the BMI2 instruction
`PEXT` (parallel bit extract):

```cpp
template <std::size_t N>
struct flat_slider {
    std::array<uint64_t, N>  data;    // the attack sets of all 64 squares, concatenated
    std::array<int, 64>      offset;  // where square sq's block starts inside data
    std::array<uint64_t, 64> mask;    // square sq's relevant-blocker mask

    uint64_t attacks(int sq, uint64_t occ) const {
        return data[offset[sq] + _pext_u64(occ, mask[sq])];
    }
};
```

`_pext_u64(occ, mask[sq])` gathers the occupancy bits that sit under the mask and packs them into
the low bits of the result. With *k* bits in the mask, that is a number from 0 to 2ᵏ − 1: a
**perfect, collision-free index** into the square's block. That's the whole lookup: one `PEXT`, one
add, one load. Unlike magic bitboards, it needs no multiplication, no magic constants and no shift.

| table | pieces | entries | size |
|---|---|---|---|
| `Lookup.diag` | bishops, queens | 5 248 | 41 KiB |
| `Lookup.rank_file` | rooks, queens | 102 400 | 800 KiB |

A queen is the union of both lookups.

### The masks

A square's mask holds the squares along its rays that could block, **excluding the last square of
each ray at the board edge**. A piece on the edge square can never block anything behind it, so
leaving it out halves the table for each ray without losing information. A rook on a1 therefore
has a 12-bit mask (b1–g1 and a2–a7), and a bishop on d4 a 9-bit mask.

### Building the tables

At start-up, for every square:

1. Build the mask as above.
2. Enumerate **every subset** of the mask, in ascending numeric order.
3. For each subset, compute the true attack set by scanning each ray until the first blocker,
   which is included, since it may be a capture.
4. Store the attack sets in that order.

Step 2's ordering is what makes the table line up with `PEXT`: for subsets of one mask, a larger
number always extracts to a larger index, so the *i*-th subset in ascending order is exactly the
subset that `PEXT` maps to index *i*. The tables are then flattened into one contiguous array per
table type, which is what `flat_slider` holds.

**Performance note:** on AMD Zen 1 and Zen 2 CPUs, `PEXT` is microcoded and slow, and every slider
lookup pays for it. From Zen 3 on, and on Intel since Haswell, it is fast: a few cycles of latency,
and one can start every cycle.

## Non-slider tables

All `constexpr`, indexed by square:

| table | contents |
|---|---|
| `knight_neighbors[sq]` | knight targets |
| `king_neighbors[sq]` | king targets |
| `pawn_neighbors[colour][sq]` | the two squares a pawn of that colour attacks |
| `pawn_movespace[colour][sq]` | the squares a pawn advances to, including the double step |
| `en_passant[sq]` | the squares adjacent to a potential en-passant target |
| `diagonals[]`, `ranks_and_files[]` | every diagonal, rank and file as a mask, used to restrict pinned pieces |
| `diagonal_pointer[sq]`, `ranks_and_files_pointer[sq]` | the diagonals and the rank/file through each square |

## Generating legal moves

`generate_moves` works in five steps.

### 1. The opponent's attack map

Every square the opponent attacks is collected into one bitboard: slider attacks, knight, pawn and
king attacks. The sliders' occupancy **leaves out the side to move's king**, so a checking ray
continues *through* the king. Without that, the king could step backwards along the checking line
and appear to be safe. The king is in check if its square is in this map.

### 2. Pins

Pins are found separately along diagonals and along ranks and files:

1. Treat the king as a slider and take its rays, stopping at the first piece in each direction.
2. Take the enemy sliders that are on the king's lines on an **empty** board: the potential pinners.
3. For each potential pinner, intersect its rays with the king's rays. If exactly one own piece
   stands between them, that piece is the intersection: it is pinned. If two or more stand
   between, the rays never meet and there is no pin.

For each pinned piece, generation records the one line (a diagonal, rank or file) it may still
move along. A piece pinned on a diagonal can make no rank or file move, and the reverse. A pinned
knight can't move at all. A pawn pinned along its file can still push, but one pinned along a
rank cannot.

### 3. Moves piece by piece

| piece | how |
|---|---|
| pawns | single pushes, double pushes (from the second rank through an empty square), captures. Reaching the last rank produces all four promotions, as quiet promotions or capturing promotions |
| en passant | the target square is in the status word. **Special case:** capturing en passant removes *two* pawns from the same rank at once. If that exposes the king to an enemy rook or queen along the rank, the capture is illegal. Ordinary pin detection can't see this, so it is checked explicitly |
| knights | `knight_neighbors`, minus own pieces (pinned knights skipped) |
| bishops, rooks, queens | `PEXT` attacks with the full occupancy, minus own pieces, restricted to the pin line if pinned |
| king | `king_neighbors`, minus own pieces and **every attacked square** from step 1 |
| castling | the right's bit is set in the status word, the squares between king and rook are empty, and the king's squares (start, crossing, destination) are not attacked. On the queen side the b-file square has to be empty but may be attacked |

### 4. Check evasion

If the king is in check, the list is then filtered:

- **Double check:** only king moves are possible, so the list is replaced by the king's moves to
  unattacked squares.
- **Single check:** a non-king move survives only if it **captures the checker** or **lands between
  the checker and the king**. An en-passant capture of a checking pawn is kept too, even though
  it lands behind the pawn rather than on it. King moves to unattacked squares are then added.

If no move survives, the position is checkmate. The search tells checkmate from stalemate using the
*in check* bit.

### 5. Output

Captures are copied to `possible_captures`, and `possible_moves` becomes the captures followed by
the quiet moves. The search does its own ordering on top of this (see [SEARCH.md](SEARCH.md)).

## Making and unmaking moves

### `make`

`make(board, move, undo)` first fills an `Undo` record with the three things a move destroys and
that can't be recomputed: the captured piece, the old status word and the old Zobrist key. It then
applies the move:

1. Clear the old en-passant targets.
2. Update the fifty-move counter: it increases after a quiet move by any piece other than a pawn,
   and resets to zero after anything else. **This includes castling**, so a castling move resets
   the counter. That's stricter than the rules require, and harmless in practice.
3. Move the pieces, with a case for each move flag: quiet, double push (which also sets the
   en-passant target, but only if an enemy pawn is placed to take it), both castlings, captures, en
   passant and promotions. Bitboards, mailbox and key are all updated incrementally.
4. Update castling rights: each rook-square bit survives only while that rook is still on its
   square **and** its king is still on e1 or e8.
5. Clear the *in check* bit (`generate_moves` sets it again for the new position) and hand the move
   to the other side.
6. Fold every changed castling or en-passant bit into the key, using the status bits selected by
   `ep_castling_mask`.

`make` leaves the move lists empty; the caller runs `generate_moves` when it needs them.

### `unmake`

`unmake(board, move, undo)` puts the pieces back, again one case per move flag, and then restores
the status word and the key **as a whole** from the `Undo` record. There's no incremental reversal
of castling rights, en-passant targets, the counter or the hash, so none of them can drift.

### Null move

`make_null` passes the turn: it flips the side to move, clears any en-passant target, and updates
the key for both. `unmake_null` restores the saved status word and key.

## Zobrist hashing

- A 64-bit key per position, built from random numbers generated once from a **fixed seed**
  (`mt19937_64`), so keys are the same on every run and every machine.
- It includes a term for each piece on each square (by colour), each castling-right bit, each
  en-passant target bit, and the side to move.
- `make` updates it incrementally; a full recomputation (`generate_initial_zobrist_key`) runs when
  a position is set up. The built-in `test` command checks that the two always agree along a
  sequence of moves.

## Repetitions and the fifty-move rule

- **Game history.** For `position … moves …`, the key after each move is pushed onto the
  repetition stack. After a move that resets the fifty-move counter (a capture, pawn move or
  castling), the stack is cleared first: no earlier position can ever occur again.
- **Search path.** The search pushes each child's key before descending and pops it on the way
  back, so the stack always holds the relevant game history followed by the current line.
- **Draw by repetition.** At every node except the root, if the current key already appears
  earlier in the stack, the node scores **0**. A single earlier occurrence is enough: if a position
  can be repeated once, it can be repeated again, so there is no need to search it. Quiescence
  nodes skip the check: they only search captures, and a capture can never repeat a position.
- **Fifty-move rule.** At every node except the root, a counter of 100 half-moves or more scores **0**.
- **Mate and stalemate.** A node with no legal moves scores *−MATE + ply* if in check, otherwise **0**.
  The ply term makes shorter mates score better.

## Verifying a build: perft

`perft depth N` counts the leaf nodes of the legal move tree. Comparing against published values
is the standard test of a move generator: any bug in castling, en passant, promotions, pins or
check evasion shows up as a wrong count.

```
position startpos
perft depth 5
```

Buster 7.0's counts, which match the published reference values for these positions:

| position | depth | nodes |
|---|---|---|
| start position | 5 | 4 865 609 |
| start position | 6 | 119 060 324 |
| "Kiwipete" `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1` | 4 | 4 085 603 |
| `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1` | 6 | 11 030 083 |
| `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1` | 5 | 15 833 292 |
| `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8` | 5 | 89 941 194 |

Perft counts the moves at the last ply without playing them ("bulk counting"), which is why it runs
at over 100 million nodes per second. The search's own speed is about 2.5 million nodes per second.
