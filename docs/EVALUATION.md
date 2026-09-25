# Evaluation

How Buster 7.0 scores a position statically. The function is `evaluation()` in `eval.cpp`. Its
weights are in `piece_table.h` (piece-square tables) and `global_constants.h` (every other term).

## Overview

- **Hand-crafted and tapered.** Every term has a middlegame and an endgame value, and the two
  totals are blended by how much material is left.
- **Side-to-move point of view.** A positive score is good for the side to move.
- **Colour-symmetric by construction.** A position and its mirror image (board flipped, colours
  swapped) score the same. The `eval` command checks this on any position.
- **About 280 lines, no hash tables.** The whole evaluation is recomputed at every call.

The steps, in order:

```
insufficient material?  → return 0 immediately
material + piece-square tables        (mg, eg)
mobility                              (mg, eg)
king safety                           (mg only)
passed pawns                          (mg, eg)
isolated and doubled pawns            (mg, eg)
bishop pair                           (mg, eg)
rooks on open and semi-open files     (mg, eg)
endgame draw rules                    (scale mg and/or eg)
blend by phase, then add the tempo bonus
```

## The taper

```
phase = (knights + bishops) + 2 · rooks + 4 · queens        counted for both sides
score = (mg · phase + eg · (24 − phase)) / 24 + tempo
```

With a full set of pieces the phase is 24, so the score is purely middlegame. It falls towards 0
(pure endgame) as pieces come off. Pawns don't count towards the phase. A term that exists only in
one half (king safety is middlegame-only) fades in or out smoothly instead of switching at a
threshold.

The phase is not capped: in the rare positions where promotions have put *more* pieces on the board
than the starting set, it exceeds 24, and the endgame half gets a small negative weight.

**Tempo:** the side to move gets a flat **+15**. A position that is completely symmetric therefore
scores exactly 15.

## Material and piece-square tables

Material is built into the piece-square tables: each table is the piece's value plus a bonus or
penalty for the square. The base values are the same in both phases:

| pawn | knight | bishop | rook | queen |
|---|---|---|---|---|
| 100 | 320 | 330 | 500 | 900 |

The shape of each table, in words:

| piece | middlegame | endgame |
|---|---|---|
| pawn | central pawns rewarded as they advance; the d2 and e2 pawns are penalised for staying home | worth more with each rank advanced, the same on every file; the 7th rank is highest |
| knight | centre good, rim and corners poor | the same idea, more strongly: a knight on the rim is worse on an open board |
| bishop | central and long-diagonal squares preferred | centre favoured |
| rook | small bonuses, the 7th rank among them | the same, plus activity |
| queen | central squares slightly preferred, edges slightly worse | safe to centralise |
| king | castled positions and the back rank preferred | centralisation: the corners are −50 |

## Mobility

For each knight, bishop, rook and queen, count the squares it attacks inside its **mobility area**:
squares not occupied by its own side and **not attacked by an enemy pawn**. Slider attacks stop at
the first piece either way. The count indexes a per-piece table:

| piece | 0 squares | middle of the table | maximum |
|---|---|---|---|
| knight (0–8) | −30 / −35 | 4 squares: +3 / +3 | 8: +17 / +18 |
| bishop (0–13) | −28 / −32 | 6: +11 / +12 | 13: +28 / +31 |
| rook (0–14) | −24 / −34 | 7: +8 / +18 | 14: +19 / +41 |
| queen (0–27) | −14 / −20 | 13: +7 / +9 | 27: +13 / +18 |

(Values are middlegame / endgame.) A trapped piece is penalised heavily, and each extra square is
worth less than the one before. Rook mobility matters much more in the endgame.

## King safety (middlegame only)

For each king, a **zone** is defined: the king's neighbouring squares plus the three squares two
ranks in front of it, towards the opponent. For each enemy piece type, count the zone squares that
at least one piece of that type attacks, and weight them:

| attacker | knight | bishop | rook | queen |
|---|---|---|---|---|
| weight per zone square | 2 | 2 | 3 | 5 |

The weighted total (the *attack units*) is turned into a penalty with a **square, capped at 500**:

```
danger = min(units² / 4, 500)      centipawns, middlegame only
```

| units | 4 | 10 | 20 | 30 | 45+ |
|---|---|---|---|---|---|
| danger | 4 | 25 | 100 | 225 | 500 |

The square is the point: a lone attacker hardly matters, but pieces converging on the king are
dangerous out of proportion to their number. The score gains the opponent's danger and loses its
own.

## Pawn structure

**Passed pawns:** a pawn with no enemy pawn ahead of it on its own file or either adjacent file.
The bonus depends on how far it has advanced, and is mostly an endgame term:

| relative rank | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|
| middlegame | 0 | 5 | 12 | 22 | 38 | 60 |
| endgame | 5 | 15 | 30 | 55 | 90 | 140 |

**Doubled pawns:** −10 / −22 for each pawn beyond the first on a file.

**Isolated pawns:** −12 / −16 for each pawn on a file with no friendly pawn on either adjacent file.
A doubled isolated pawn is penalised both ways.

## Pieces

| term | middlegame | endgame |
|---|---|---|
| bishop pair (two or more bishops) | +20 | +35 |
| rook on an open file (no pawns of either colour) | +25 | +15 |
| rook on a semi-open file (no own pawns) | +12 | +8 |

## Endgame knowledge

Four rules recognise endings where the material count lies. Three of them **scale** the score
instead of setting it: a scale of *s*/64 multiplies the score by *s*/64 before the blend.

| rule | when | effect |
|---|---|---|
| **Insufficient material** | no pawns, rooks or queens, and each side has at most one minor piece, or one side has two knights against a bare king | the evaluation returns **0** at once |
| **Opposite-coloured bishops** | the only pieces besides kings and pawns are one bishop each, on opposite colours, and the pawn counts differ by at most one | endgame score × **8/64** |
| **Wrong-coloured rook pawn** | only bishops and pawns on the board; the stronger side has at most one bishop and pawns only on the a-file or only on the h-file; the defender has no bishop and no pawns; the bishop, if any, can't cover the promotion square; the defending king is within 3 squares of that square | both halves × **4, 4, 16, 32 /64** at distance 0, 1, 2, 3 |
| **Pawnless small edge** | the side the evaluation favours has **no pawns** and is ahead by at most a minor piece (counting knight or bishop 3, rook 5, queen 9), except two bishops against a knight | both halves × **0/64** if that side has less than a rook, **4/64** if the defender has at most a minor piece, otherwise **14/64** |

The reasoning behind the design:

- **Scaling, not a cliff.** Apart from insufficient material, which is a true dead draw, these
  positions are drawish *by degree*. A hard switch to 0 at a boundary would let the score jump
  between two neighbouring positions and send the search chasing that jump.
- **Opposite-coloured bishops scale only the endgame half.** A big pawn majority can still win, so
  the rule stops at a pawn difference of one, and it leaves the rest of the evaluation alone.
- **The rook-pawn and pawnless rules scale both halves.** These positions are simply drawn or close
  to it. Because the phase still gives them some middlegame weight, scaling only the endgame half
  would leak part of the unscaled score through.
- **The wrong-rook-pawn rule allows zero bishops.** If it required exactly one, giving up the
  bishop would escape the rule, and the search would be rewarded for throwing a piece away.
- **The pawnless rule only ever reduces a pawnless side's advantage.** If the side with pawns is
  winning, the evaluation favours that side and the rule doesn't apply.

Examples of the effect (static evaluation, side to move White):

| position | without the rule | with it |
|---|---|---|
| K + B vs K | about +380 | 0 |
| K + R vs K + B | about +175 | about +25 |
| K + B vs K + P | about +240 | 15 (just the tempo) |
| K + B + a-pawn vs K, wrong bishop, king in the corner | about +540 | under 60 |

## Checking the evaluation

The `eval` command prints the static evaluation of the current position, then evaluates the
mirrored position and prints the difference:

```
position startpos
eval
eval 15 (cp, side-to-move POV) mirror 15  diff 0  OK
```

Any non-zero difference means a term treats the colours differently, which is always a bug.
`test` repeats the mirror check across many positions reached by playing out moves.

## What the evaluation does not have

No pawn-structure hash table (all pawn terms are recomputed every time), no term for hanging or
attacked pieces (the search and SEE handle those), no king shelter or pawn-storm term, no outposts,
no mating-pattern knowledge for won endings, and no neural network. Several of these were tried and
measured; [TESTING.md](TESTING.md) records what happened.
