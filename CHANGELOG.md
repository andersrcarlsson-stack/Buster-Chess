# Changelog

## 7.0.1 — 2026-09-25

First public release. A bug-fix release on top of 7.0: search and evaluation are unchanged, so the
7.0 rating applies.

- **No crash on malformed numbers.** A non-numeric value in `setoption`, in any `go` parameter,
  in `perft` or `test`, or an out-of-range half-move count in a FEN used to terminate the engine.
  Each is now ignored with an `info string` warning. `perft depth` with no number no longer reads
  past the end of the command.
- **Castling notation:** the `info string` move printed O-O for queen-side castling and O-O-O for
  king-side castling. `bestmove` was always correct.
- An unknown option name is now reported as unknown instead of as set.
- A FEN half-move count above 100 is treated as 100 (it was silently wrapped above 127).

## 7.0 — 2026-09-25

Rated about **2757 CCRL Blitz** (see [docs/TESTING.md](docs/TESTING.md)).

Changes from the previous (unreleased) version:

- **Time management** rewritten with a soft limit (no new iteration once half the target time is
  spent) and a hard limit (three times the target, at most half the clock). Before, about 31% of each
  move's time was spent on iterations that were then thrown away; now almost none is. **+74 Elo in
  self-play**, and the main reason 7.0 rates about 65 points above its predecessor.
- **Standard `info` output:** one line per iteration with depth, selective depth, score, nodes,
  speed, hash use, time and a principal variation. GUIs and match runners now show depth and PV.
- **`Hash` option** (1–4096 MiB).
- **Endgame knowledge:** a pawnless side ahead by at most a minor piece (for example rook against
  bishop) is now recognised as drawish. This completes the four endgame draw rules.
- **Fixes:**
  - the engine no longer crashes if a GUI sends `position` or `ucinewgame` while it is still searching;
  - it now exits cleanly when its input is closed;
  - a promotion given with an unknown piece letter no longer uses an uninitialised value;
  - it no longer waits for typed input when `position` arrives before `uci`.
- **Build:** zero warnings under `-Wall -Wextra`, and zero AddressSanitizer / UndefinedBehaviorSanitizer
  reports; `make check` and `make asan` targets.
- Licensed under **MIT**.
