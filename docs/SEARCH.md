# Search

How Buster 7.0 decides on a move: iterative deepening around a negamax alpha-beta search, a
quiescence search at the leaves, the transposition table, move ordering, and the rules that cut,
reduce and extend the tree. Every number quoted here is the 7.0 default. Most of them are exposed
as UCI options (see [USAGE.md](USAGE.md)).

## Overview

```
search()                        iterative deepening, one iteration per depth
 └─ aspiration loop             re-search with a wider window on fail low / fail high
     └─ alpha_beta(depth, ply)  negamax, fail-soft, full-window re-searches (no PVS)
         └─ q_search(ply)       captures only, at depth 0
```

Scores are centipawns from the side to move's point of view. Checkmate is **±32 000**, adjusted by
the distance from the root, so a shorter mate scores better. Any score beyond ±31 872 is a mate
score. Draws score **0**; there is no contempt.

## Iterative deepening

`search()` runs depth 1, 2, 3, … up to the requested maximum (40 by default). After each completed
iteration it prints one `info` line with depth, selective depth, score, nodes, speed, hash use, time
and principal variation, and remembers the iteration's best move.

- **Stopping.** The search stops when the time manager says so (see below), at the node limit, or
  when the GUI sends `stop`. An iteration that is interrupted is **thrown away**, and `bestmove` is
  the best move of the last *completed* iteration.
- **Principal variation.** The PV is read from the transposition table after each iteration:
  starting from the best move, it follows each position's stored best move, checking each move is
  legal before playing it. It stops at a missing entry or after as many moves as the iteration's
  depth, so the tail of a PV can come out short.
- **Safety net.** Before `bestmove` is printed, the move is checked against the root's legal moves.
  If it is somehow not among them, the first legal move is played instead. A position with no
  legal moves at all answers `bestmove 0000`.

## Aspiration windows

From depth **3** (`AspMinDepth`), an iteration starts with a narrow window around the previous
iteration's score, **±16** centipawns (`AspDelta`), instead of the full window.

- **Fail low** (score ≤ α): move α down by the current δ below the score, keep β, and double δ.
- **Fail high** (score ≥ β): move β up the same way, keep α, and double δ.
- Repeat until the score lands inside the window.

After a mate score, the next iteration uses the full window.

## Alpha-beta

`alpha_beta` is a fail-soft negamax. It does not use a principal-variation search (PVS): every
move, including a re-searched reduced one, is searched with the full (α, β) window. At each node,
in this order:

1. **Draw checks,** everywhere except the root: a repetition of any earlier position on the game
   or search path scores 0, and so does a fifty-move counter of 100 half-moves or more (see
   [MOVEGEN.md](MOVEGEN.md)).
2. **No legal moves:** −32 000 + ply if in check (checkmate), otherwise 0 (stalemate).
3. **Depth 0:** hand over to quiescence if there are captures, or return the static evaluation if
   there are none.
4. **Transposition table.** If the entry was searched at least as deep as this node, it can end the
   node: an exact score is returned; a lower bound ≥ β or an upper bound ≤ α is returned too.
   Otherwise its move is still used for ordering.
5. **Static evaluation,** computed once when remaining depth ≤ 8 and the side to move isn't in
   check. It is needed only by the two futility rules below.
6. **Reverse futility pruning,** below.
7. **Move ordering,** below.
8. **Null-move pruning,** below.
9. **The move loop:** futility and late-move pruning, then make the move, extend or reduce, and
   search it.
10. **Store** the result in the transposition table: exact, lower or upper bound, with the move
    that produced it.

### Move ordering

Moves are tried in five groups:

| order | group | how it is ordered |
|---|---|---|
| 1 | **hash move** | the transposition table's best move for this position, if it is legal |
| 2 | **winning and equal captures** | static exchange evaluation (SEE) ≥ 0, highest first |
| 3 | **losing captures** | SEE < 0, least bad first |
| 4 | **killer moves** | up to two quiet moves that caused a cutoff at this ply elsewhere in the tree |
| 5 | **remaining quiet moves** | best score first, where score = history + continuation history |

**SEE** plays out the whole exchange on the target square, both sides recapturing with their
least valuable attacker each time, and reports the material balance. It includes x-rays: a slider
behind another attacker joins once the piece in front of it has captured. Piece values for SEE:
pawn 100, knight 320, bishop 330, rook 500, queen 900. Pins are ignored, as in any SEE.

**Quiet moves are selected lazily.** Their scores are taken once when the node starts, and each
time the loop reaches the quiet section it picks the best remaining move with a linear scan. A
node that cuts off after a few quiets never pays to sort the rest.

**History** (`[side][from][to]`) and **continuation history** (`[side][previous piece][previous
to-square][piece][to-square]`, i.e. conditioned on the opponent's last move) are updated whenever a
**quiet** move causes a beta cutoff:

- The cutoff move gets a bonus of depth², and every quiet move searched before it at that node gets
  the same amount as a penalty. Continuation history is updated only when there is a previous move
  to condition on, so not at the root and not right after a null move.
- Each update uses **gravity**: `entry += bonus − entry·|bonus| / 16384`. That keeps every entry
  within ±16 384 without ever resetting the tables, and it lets old information fade.
- The cutoff move also becomes this ply's first killer; the previous first killer becomes the second.

History, continuation history and killers persist from move to move within a game and are cleared
by `ucinewgame`.

### Pruning, reductions and extensions

| technique | when | what | default |
|---|---|---|---|
| **Reverse futility pruning** | remaining depth ≤ `RfpDepth`, not in check, β not a mate score | if static eval − `RfpMargin` · depth ≥ β, return the static eval without searching | depth 8, margin 70 |
| **Null-move pruning** | not at the root, not after another null move, not in check, depth ≥ 3, side to move has a knight, bishop, rook or queen, β not a mate score | pass the turn; search depth − 1 − R with a null window around β. If the result is still ≥ β, the node fails high | R = 2 (`NullMoveR`) |
| **Futility pruning** | depth ≤ `FutilityDepth`, not the first move, not in check, α not a mate score, not a promotion, a quiet move or a losing capture | skip the move if static eval + `FutilityMargin` · depth ≤ α | depth 5, margin 106 |
| **Late-move pruning** | depth ≤ `LmpDepth`, quiet move past the killers, not in check, α not a mate score, not a promotion | after 3 + depth² such quiets have been searched, skip all remaining moves | depth 3 |
| **Late-move reductions** | depth ≥ 3, not in check, the move doesn't give check, a quiet move past the killers or a losing capture | search at depth − 1 − r. If the result beats α, search again at full depth | table below |
| **Check extension** | the move gives check (and is not reduced) | search one ply deeper | +1 (`CheckExt`) |

The two groups that are **never** pruned or reduced by move type are the hash move and the SEE ≥ 0
captures. Killers are exempt from late-move pruning and late-move reductions.

A null-move search that returns a mate score doesn't pass that mate on: the node returns β. A mate
found only because one side "passed" is not a real mate.

**Late-move reduction table.** `r = ⌊0.75 + ln(depth) · ln(move number) / 2.25⌋`, capped at
depth − 2, where the move number is the move's position in the ordered list (0 = first move). Some
values:

| depth \ move number | 2 | 4 | 8 | 16 | 32 | 63 |
|---|---|---|---|---|---|---|
| 3 | 1 | 1 | 1 | 1 | 1 | 1 |
| 4 | 1 | 1 | 2 | 2 | 2 | 2 |
| 6 | 1 | 1 | 2 | 2 | 3 | 4 |
| 8 | 1 | 2 | 2 | 3 | 3 | 4 |
| 12 | 1 | 2 | 3 | 3 | 4 | 5 |
| 16 | 1 | 2 | 3 | 4 | 5 | 5 |
| 24 | 1 | 2 | 3 | 4 | 5 | 6 |

## Quiescence search

At depth 0, `q_search` keeps resolving captures so the evaluation is never taken in the middle of an
exchange:

1. **Transposition table.** Any matching entry can end the node, with the same bound rules as above
   but **no depth condition**: quiescence entries are stored at depth 0, and any deeper entry is at
   least as good.
2. **No legal moves:** checkmate or stalemate score, as in the main search.
3. **Stand pat.** The static evaluation is a floor: the side to move can usually decline to capture.
   If it is already ≥ β, return it; otherwise raise α to it.
4. **Captures in SEE order,** skipping:
   - **losing captures** (SEE < 0), except promotions;
   - **delta pruning:** a capture that couldn't lift the score to α even if it won the captured
     piece for free plus a margin: stand pat + value of the captured piece + `DeltaMargin` (190) ≤ α.
     Not applied in check or to promotions.
5. **Store** the result at depth 0.

Quiescence searches captures and capturing promotions only. Quiet promotions and quiet moves that
give check are left to the main search.

## Time management

For a game with a clock, the `go` command's times are turned into two limits:

```
target = my_time / 20 + ¾ · increment        (divide by min(movestogo, 20) when movestogo is given)
soft   = target / 2
hard   = min(3 · target, my_time / 2)
```

- **Soft limit, checked between iterations.** Once the time spent passes `soft`, no new iteration
  starts. The tree grows by a factor of about 1.8 per ply, so the next iteration would cost about
  0.8 times *all* the time spent so far, and one started after that point would almost never finish.
- **Hard limit, checked during the search.** It aborts an iteration that runs long, usually one
  whose score just dropped and which is searching for a better move. The high hard limit gives
  exactly those iterations room to finish.
- **Result:** Buster uses about **0.7 × target** per move on average, and practically none of it is
  spent on iterations that are then thrown away.
- `go movetime T` uses T as both limits. `go depth`, `go nodes` and `go infinite` stop only on their
  own limit or on `stop`.

## What Buster does not use

For readers comparing Buster with other engines: 7.0 has **no** principal-variation search, internal
iterative deepening, singular extensions, razoring, ProbCut, "improving"-based margins, check
evasions in quiescence, pondering, multi-threading or endgame tablebases. Several of these were
tried and measured; [TESTING.md](TESTING.md) records what happened.
