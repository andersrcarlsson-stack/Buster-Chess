# Usage

How to run Buster 7.0.1: in a GUI, from a match runner, or by typing UCI commands directly. This is
the reference for every command, `go` parameter and option the engine understands.

## Quick start

```
make                # builds ./buster
make check          # confirms the build: self-tests and a perft count
./buster            # starts the engine; it waits for UCI commands on standard input
```

**In a GUI** (Cute Chess, Arena, Banksia, En Croissant, …): add a new engine, choose the **UCI**
protocol, and point it at the `buster` binary. The GUI discovers the options on its own.

**In a match runner** (fastchess, cutechess-cli): for example

```
fastchess -engine cmd=./buster name=Buster -engine cmd=./other name=Other \
          -each tc=60+1 option.Hash=128 -rounds 100
```

**By hand**, a minimal session:

```
uci
isready
position startpos moves e2e4 e7e5
go movetime 2000
```

Buster answers `id`, the options and `uciok`, then `readyok`, then an `info` line per completed
iteration, and finally `bestmove`. The search runs in the background, so `stop` or any other
command can be sent while it is thinking.

## UCI commands

| command | what Buster does |
|---|---|
| `uci` | prints `id name Buster 7.0.1`, `id author`, the options, `uciok` |
| `isready` | prints `readyok` |
| `ucinewgame` | clears the transposition table, killers, history and continuation history |
| `position startpos [moves …]` | the start position, then the moves in UCI notation (`e2e4`, `e7e8q`) |
| `position fen <6 fields> [moves …]` | a position in FEN, then moves |
| `go …` | starts a search; see below |
| `stop` | ends the search; the best move so far is printed as `bestmove` |
| `setoption name <name> value <n>` | sets an option; see below |
| `quit` | stops any search and exits |
| `debug`, `register`, `ponderhit` | accepted and ignored |

- **Invalid FEN.** A malformed FEN is rejected with an `info string` saying what is wrong, and the
  previous position stays in place. A `go` sent after a rejected `position` is ignored.
- **One search per position.** After a `go`, send a new `position` before the next `go`; a second
  `go` for the same position is ignored. GUIs always do this.
- **Commands during a search.** `position`, `ucinewgame`, `setoption` and `go` first stop any
  running search, which prints its `bestmove`, and then take effect.
- **Unknown words.** Buster looks for the first recognised command on a line and ignores anything
  before it. A line with no recognised command is ignored.

### `go`

| parameter | default | meaning |
|---|---|---|
| `wtime`, `btime` | — | milliseconds left on White's and Black's clocks |
| `winc`, `binc` | 0 | increment per move, in milliseconds |
| `movestogo` | 0 (sudden death) | moves until the next time control |
| `movetime` | — | search for exactly this many milliseconds |
| `depth` | 40 | stop after this depth; with a clock it is only an upper limit |
| `nodes` | — (unlimited) | stop after this many nodes; this makes the search **deterministic** |
| `infinite` | — | search until `stop`, or until depth 40 is completed (in practice, hours) |

Without `wtime`/`btime`, `movetime` or `nodes`, Buster searches until `stop` or until the depth
limit (40 unless `depth` is given) is completed. With a clock,
it allots its own time: see "Time management" in [SEARCH.md](SEARCH.md). `ponder` is not supported.

## Output

After every completed iteration:

```
info depth 16 seldepth 27 score cp 15 nodes 4172273 nps 2091364 hashfull 147 time 1995 pv e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 g8f6 b1c3 f8b4 c1g5 d7d5 f1b5 b4c3 b2c3 d5e4
```

| field | meaning |
|---|---|
| `depth` | the iteration just completed |
| `seldepth` | the deepest ply reached, including quiescence |
| `score cp N` | centipawns from the side to move's point of view |
| `score mate N` | mate in N moves; negative if Buster is being mated |
| `nodes`, `nps`, `time` | nodes searched so far, nodes per second, milliseconds since `go` |
| `hashfull` | transposition-table use in permille (sampled) |
| `pv` | the expected line, read from the transposition table; it can end early |

When the search ends:

```
info string e4
bestmove e2e4
```

The `info string` repeats the move in standard algebraic notation for people reading the output.
`bestmove 0000` means the position has no legal moves (checkmate or stalemate).

## Options

| option | type | default | range | effect |
|---|---|---|---|---|
| `Hash` | spin | 128 | 1–4096 | transposition-table size in MiB |

`Hash` is rounded **down** to a power of two, and Buster reports what it allocated:
`setoption name Hash value 100` answers `info string Hash 64 MiB`. Changing it clears the table.
Bigger isn't automatically better: 128 MiB is plenty for blitz, and 256 MiB is enough for longer
games. When running several engines at once, remember that each instance allocates its own table.

**Search parameters.** These options exist so the search can be tuned automatically. The defaults
are the tuned values, so **leave them alone for play**. SEARCH.md describes what each one controls.

| option | default | range |
|---|---|---|
| `FutilityMargin` | 106 | 0–500 |
| `FutilityDepth` | 5 | 0–8 |
| `RfpMargin` | 70 | 0–500 |
| `RfpDepth` | 8 | 0–10 |
| `DeltaMargin` | 190 | 0–500 |
| `LmpDepth` | 3 | 0–8 |
| `NullMoveR` | 2 | 1–4 |
| `CheckExt` | 1 | 0–1 |
| `AspDelta` | 16 | 5–200 |
| `AspMinDepth` | 3 | 2–8 |
| `LmrReduction` | 2 | 0–3 |

`LmrReduction` has **no effect** in 7.0; late-move reductions come from a table (see SEARCH.md). It
is kept so tuning setups written for older versions still work.

Option values must be whole numbers. A value that isn't one is ignored with an `info string`
warning, and so is an unknown option name. Buster has a single search thread, so there is no
`Threads` option.

## Extra commands

Buster adds four commands for testing and development. GUIs never send them.

### `perft depth N`

Counts the leaf nodes of the legal move tree from the current position to depth N (the word
`depth` is required), and prints the count and the time:

```
position startpos
perft depth 5
The number of nodes found: 4865609
Time used for generation; 33  milliseconds
```

[MOVEGEN.md](MOVEGEN.md) lists the counts for six standard test positions. Any difference means
the move generator is broken.

### `eval`

Prints the static evaluation of the current position, from the side to move's point of view, then
evaluates the mirror-image position (board flipped, colours swapped) and prints the difference.
The difference must be 0:

```
eval 15 (cp, side-to-move POV)
mirror 15  diff 0  OK
```

This is the static evaluation only, with no search. A symmetric position reads exactly 15, the
tempo bonus.

### `test [depth]`

Runs the built-in self-tests on the current position:

- unit checks of hashing, move encoding, the fifty-move counter, perft counts, and the colour
  symmetry of every evaluation table;
- then a walk of **every** legal move sequence to the given depth (default 4). At every position
  it checks that the incremental hash equals a full recomputation, that `make` followed by `unmake`
  restores the board exactly, that the evaluation is colour-symmetric, and that the null move
  hashes and restores correctly.

```
position startpos
test 3
tests: 46634/46634 passed
```

The default depth runs just over a million checks (1 033 039) from the start position in about a
quarter of a second. Failures
are listed (up to ten) before the summary line.

### `position kiwipete`

A shortcut for the well-known "Kiwipete" test position
(`r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -`), which exercises castling,
en passant, promotions and pins in very few moves.

## Limits

- **One search thread.** No `Threads` option, no pondering.
- **No endgame tablebases.**
- **BMI2 required** (see the README).
- **Maximum depth:** 40 by default. Killers and the continuation context cover 64 plies from the root.
