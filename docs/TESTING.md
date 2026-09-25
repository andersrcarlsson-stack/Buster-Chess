# Testing

How Buster was built and validated: the correctness checks, how strength was measured, what each
feature was worth, and which ideas were tried and **rejected**. The development test scripts are not
part of this repository. This document describes the method and records the results.

## The rule

**No feature is kept on belief.** A change stays only if it passes one of two tests:

- a **strength test:** a statistically decisive self-play match against the previous version; or
- a **correctness gate,** for changes whose whole purpose is to make the engine right rather than
  stronger, such as a bug fix or an endgame draw rule.

Changes that are sound but whose effect is too small to measure are kept "on mechanism" and are
**not** credited with any Elo.

## Correctness gates

| gate | what it catches |
|---|---|
| **perft** on standard positions | any move-generation error (see [MOVEGEN.md](MOVEGEN.md)) |
| **`test`** self-tests | hashing, make/unmake, null-move and evaluation-symmetry errors over every position in a move tree (see [USAGE.md](USAGE.md)) |
| **fixed-depth search scores** on a few positions | an unintended change in search results |
| **node-count signature**: nodes, selective depth, score and best move at fixed depth on five positions | *any* behaviour change. A refactor or a speed optimisation must leave it **bit-identical** |
| **endgame suite:** 22 positions with a known verdict (draw, win, or a bound on the score) | endgame rules that fire too widely or not at all |
| **sanitizer sweep** (AddressSanitizer, UndefinedBehaviorSanitizer) | memory errors and undefined behaviour; the baseline is **zero reports** |
| **compiler warnings** (`-Wall -Wextra`) | the baseline is **zero warnings** |

The two zero baselines are deliberate. When nothing is reported, a single new report is a signal
instead of noise.

## Measuring strength

### Self-play SPRT

Each change is played as **new version against old version**:

- **Openings** from an unbalanced book (UHO), each played twice with colours reversed, so the book's
  bias cancels out.
- **Sequential probability ratio test** with bounds elo0 = 0, elo1 = 5 and α = β = 0.05. The match
  stops as soon as the result is statistically decided.
- **At most four games at once** on the test machine, so the engines never compete for CPU.

**The limit must match the mechanism:**

| change | limit | why |
|---|---|---|
| search, move ordering, pruning | **fixed nodes** | deterministic and reproducible; measures how well the nodes are used |
| evaluation, speed, time management | **fixed time** | fixed nodes would give a slower evaluation free extra time and hide its cost |

**Reading a result:**

- If the confidence interval lies entirely above elo1, the change is a gain and the match can stop,
  even before the formal test statistic crosses its bound (it lags behind).
- If the interval contains 0, the result is inconclusive.
- A real effect smaller than about 5 Elo never converges; the test just wanders. Such a change is kept
  only if the mechanism is sound, and gets no Elo figure.

### Speed

A speed optimisation must not change behaviour, so its node counts at fixed depth must be identical.
Its effect is measured as a **paired, interleaved** comparison of nodes per second: A, B, A, B, …
alternating runs so thermal and background noise hit both versions equally. Two separate
measurements taken minutes apart differed by 1.6% on identical binaries; the paired method removes
that drift.

### Absolute rating

Self-play Elo overstates the effect a change has against other engines. The absolute number comes
from a separate **gauntlet**:

- Buster plays four engines that are on the current CCRL Blitz rating list, at the exact versions
  listed, at CCRL's blitz time control of 2 minutes + 1 second per move.
- **Ordo** then fixes each opponent at its published rating and solves for Buster's.
- Only opponents Buster scores between 20% and 80% against are trusted. Outside that range the rating
  maths becomes unreliable.

**Buster 7.0: about 2757 CCRL Blitz** (320 games, 2026-09-25). The four opponents individually imply
2701 to 2820, so the honest reading is a band of roughly **2700–2820**. Ordo's own ±28 error bar is
purely statistical and does not include the disagreement between the anchors.

## What each feature was worth

Self-play results, each against the version just before it. The numbers are **not additive**: each
was measured on a different engine, and later features partly overlap earlier ones. Early results
at small node budgets overstate the gain.

### Search

| feature | Elo | limit |
|---|---|---|
| stand-pat cut-off in quiescence | +252 (inflated by a small node budget) | nodes |
| transposition-table bound flags | +93 | nodes |
| hash move first | +65 | nodes |
| killer moves | +41 | nodes |
| history heuristic | +16 | nodes |
| null-move pruning | +77 | nodes |
| late-move reductions | +57 | nodes |
| SEE ordering of captures | +15 | nodes |
| SEE-based choice of which moves to reduce | +11 | nodes |
| correct in-check detection (made null-move pruning sound) | +42 | nodes |
| check extension | +30 | nodes |
| futility pruning | +82 | nodes |
| automatic parameter tuning (SPSA), first round | +56 | nodes |
| lazy selection of quiet moves | +24 | time |
| late-move pruning | +47 | nodes |
| reverse futility pruning | +46 | nodes |
| late-move reduction table (depth × move number) | +25 | nodes |
| aspiration windows | +50 | nodes |
| transposition table in quiescence | +19 | nodes |
| automatic parameter tuning, second round | +7 (small, not decisive) | nodes |
| continuation history | +9 (small, not decisive) | nodes |
| **time management: soft and hard limits** | **+74** | time |

### Evaluation

| feature | Elo | limit |
|---|---|---|
| tapered evaluation (middlegame/endgame blend) | +108 | time |
| mobility | +75 | time |
| king safety (attack units) | +65 | time |
| passed pawns | +26 | time |
| pawn structure and bishop pair | +32 | time |

### Kept on mechanism (no Elo claimed)

Losing-capture pruning in quiescence, SEE refinement of futility pruning, rooks on open files and
the tempo bonus, true ply tracking (correct mate distances and killer slots), delta pruning, a fix to
static exchange evaluation, and the four endgame draw rules. The draw rules were validated by the
endgame suite: endgames are too rare in self-play for an SPRT to see them.

## Tried and rejected

These ideas were measured and did **not** make it into 7.0. The reasons are often more useful than
the results.

| idea | result | why it failed |
|---|---|---|
| **Principal-variation search** | lost three times (−59, −45, −46) | Correctly implemented: with pruning disabled it gave identical scores. But reverse futility pruning, which cuts without searching, misbehaves inside PVS's null-window searches. It would need the pruning margins re-tuned together with PVS |
| **Transposition-table buckets with ageing** | no gain (−3 ± 19), even with a deliberately tiny table | the depth-preferred replacement rule already had the benefit; at normal sizes the table rarely has to evict anything useful |
| **Counter-move heuristic** | no gain | it overlaps killers and history, and exempting its moves from reductions made tactical trees much larger |
| **Two-ply continuation history** | no gain (−2) | too sparse: each entry is updated too rarely to learn anything at blitz depths |
| **King shelter, pawn storm and open files near the king** | −27; then 0; then −23 as a scaled variant | the existing attack-unit term already rises sharply when pieces converge on an exposed king, so the extra terms counted the same danger twice |
| **Pruning quiet moves that lose material (SEE < 0)** | inconclusive (+8 ± 11) | late-move pruning already removes almost exactly those moves |
| **Piece-square difference as a quiet-move ordering key** | enlarged the tree in all seven variants tried | a fixed table displaced history, which is learned from actual cutoffs in the current search |
| **Automatic tuning of the evaluation weights (Texel method)** | −672, then −84, then −35 | minimising the prediction error on game results is not the same as maximising strength; even sensible-looking weights lost |
| **A complete handcrafted evaluation built from a published evaluation compendium** (about 90 terms) | about −470 at equal nodes | more knowledge, individually plausible, but the terms had never been tuned to work together, and the evaluation was 43% slower |
| **Compiler options:** profile-guided optimisation alone; `-DNDEBUG`, no stack protector | −1.7%; 0% speed | removing instructions is not the same as saving time. Profile-guided plus link-time optimisation gave +3.7% but was left out so that every measurement stays reproducible |

**The pattern across the evaluation and ordering results:** signals that are *measured* (history,
learned from cutoffs in the running search; search parameters tuned by playing games) beat signals
that are *written by hand or fitted indirectly* (piece-square ordering, extra king terms, a knowledge
compendium, weights fitted to game results). The remaining evaluation gains for an
engine at this level most likely need a new kind of mechanism, such as a neural-network evaluation,
not more hand-written terms.

## Method notes

Principles that came out of this work and applied everywhere:

- **Check with a cheap test before an expensive one.** A move-ordering change must give the *same*
  score with *fewer* nodes at fixed depth before it earns a match.
- **Predict the number before measuring it.** Several scaling errors were caught only because a
  hand-calculated prediction disagreed with the measurement. The mirror test can't catch a term with
  the wrong *scale*, only one that is asymmetric.
- **When prediction and measurement disagree, find out why.** Don't explain it away.
- **Measure the evaluation's scale over thousands of positions, not a handful.** An evaluation that
  looked right on seven positions was 46% too large over eight thousand.
- **Fix known bugs before measuring.** A known bug doesn't just add noise: it produces a confidently
  wrong verdict.
- **A rule that works at the root may fire deep in the tree.** An endgame rule that could never apply
  in the test positions themselves still changed the search, because positions deep in the search
  tree met its conditions.
