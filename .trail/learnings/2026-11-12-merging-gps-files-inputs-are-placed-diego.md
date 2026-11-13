---
title: "Merging gps files: inputs are placed within 12 hours of the previous one, since the gps time has no date"
author: Diego
date: 2026-11-12
branch: v0.84-todo-fixes
cost: { minutes: 27, tokens: 231913 }
id: l-a83ef12d
scope: repo
status: active
src: agent
touches:
  - tests/gps/check_join.py
  - CLAUDE.md
  - src/utils/joingps_s.c
  - src/utils/gps_stream.h
---

## Decided
- The gps time in each record's trailer is the time of day in 1/128 s. It wraps to 0 at midnight and the .gps header carries no date. Merging only by that time put everything after 00:00 from the first input to cross midnight ahead of what the other inputs still had before 00:00, and a file starting at 23:59 came out after a file starting at 00:00.
- Within one input, a drop of more than 12 h is a midnight crossing (add a day). Across inputs, the first record of each input is placed within 12 h of the previous input's first record on the command line. To join several days, the inputs must be given in chronological order; a glob over YYMMDD names already does that.
- On equal times the later input goes first, as with two files before, so two-file output without midnight stays byte-identical to the old binary.

## Tried and ruled out
- Choosing the day from the largest gap between start times: it misorders a day split into uneven pieces (00-06, 06-08, 08-20, 20-24), because the largest gap among start times is not where the day begins.
- Using only the general heap for two inputs: with heavily interleaved files the input changes on almost every record, and the extra work per change (heap indexing, the two-step tie-break, jg_advance not being inlined) cost about 10 % CPU. Instruction counts were nearly equal; the tie-break in two comparisons raised branch mispredictions by 24 %.

## Watch out
- Any tool that compares record times must handle midnight. The ambiguity between inputs cannot be fully resolved without a date: an input at 00:00 of the same day as one at 23:59 is taken as the next day.
- In the code: src/utils/joingps_s.c (jg_advance, jg_merge2, ord = key*nin + rank as a single comparison, JG_HOT always_inline guarded for gcc >= 3.1); tests/gps/check_join.py checks an output against an independent reference merge.
