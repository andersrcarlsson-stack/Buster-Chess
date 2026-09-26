# Buster

A UCI chess engine in C++, by Anders R Carlsson.

Buster uses `PEXT` bitboards, a negamax alpha-beta search and a hand-crafted tapered evaluation. It
speaks the UCI protocol, so it runs in any UCI GUI (Cute Chess, Arena, Banksia, En Croissant, …) and
in match runners such as fastchess and cutechess-cli.

**Strength:** about **2757 CCRL Blitz** for version 7.0 (Ordo, 320 games at 2'+1" against four
engines on the current CCRL Blitz list, 2026-09-25). The four opponents individually imply 2701–2820,
so read it as roughly **2700–2820**, not as a precise figure.

## Building

Buster **needs a CPU with BMI2** (the `PEXT` instruction): Intel Haswell (2013) or newer, or AMD Zen.
On **AMD Zen 1 and Zen 2** `PEXT` is microcoded and very slow, so Buster runs there, but at a fraction
of its speed. Zen 3 and later are fine.

```
make            # builds ./buster with g++ (C++20), -O3 -march=native
make check      # self-tests plus a perft count: confirms the build is correct
```

`-march=native` optimises for the machine you compile on. To build a binary for other machines:
`make CXXFLAGS="-std=c++20 -O3 -march=haswell -mbmi2"`.

**Prebuilt binaries** for Linux (x86-64, fully static) and Windows (x86-64) are on the GitHub
**Releases** page. They are built with `-march=haswell`, so they run on any CPU with BMI2. To build
them yourself: `make release` (the Windows one needs the MinGW-w64 cross-compiler).

## Using it

Point your GUI at `./buster` and choose the UCI protocol. The only option you need is `Hash`
(transposition-table size in MiB, default 128). The other options are search parameters, exposed
for tuning. **Leave them at their defaults for play.**

Buster is single-threaded, does not ponder, and has no endgame tablebases.

## What is inside

- **Move generation:** fully legal, on bitboards, with slider attacks through `PEXT` lookup tables
  and pins resolved during generation.
- **Search:** iterative deepening with aspiration windows; fail-soft alpha-beta; quiescence with SEE
  and delta pruning; a transposition table with full-key verification; staged move ordering (hash
  move, captures by SEE, killers, quiets by history and continuation history); null-move, reverse
  futility, futility and late-move pruning; late-move reductions; check extension; time management
  with soft and hard limits.
- **Evaluation:** tapered middlegame/endgame piece-square tables, mobility, king safety (attack
  units), passed pawns, pawn structure, bishop pair, rooks on open files, tempo, and four endgame
  draw rules.

Every feature was kept only after a statistically decisive self-play test or a correctness gate.

## Documentation

| document | contents |
|---|---|
| [docs/USAGE.md](docs/USAGE.md) | every UCI command, `go` parameter and option, the output format, the extra test commands |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | modules, data structures, the move and status-word formats, threads |
| [docs/MOVEGEN.md](docs/MOVEGEN.md) | `PEXT` attacks, legal move generation, make/unmake, hashing, perft |
| [docs/SEARCH.md](docs/SEARCH.md) | the search, move ordering, every pruning rule with its parameters, time management |
| [docs/EVALUATION.md](docs/EVALUATION.md) | every evaluation term and its weights, the endgame rules |
| [docs/TESTING.md](docs/TESTING.md) | how Buster was tested, what each feature was worth, and what was tried and rejected |
| [CHANGELOG.md](CHANGELOG.md) | release notes |

## Licence

MIT. See [LICENSE](LICENSE).
