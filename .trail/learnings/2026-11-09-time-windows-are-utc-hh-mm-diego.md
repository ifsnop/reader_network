---
title: "Time windows are UTC HH:MM and include the whole last second"
author: Diego
date: 2026-11-09
standing: true
id: l-e4633bed
scope: repo
status: active
src: init-sessions
touches:
  - src/utils/filtertime_s.c
---

## Decided
- strptime plus mktime used local time, so ranges shifted on any machine not in UTC. The end bound also dropped nearly one second, so chained windows lost records at each edge (99 at 100 records/s).
- In the code: In src/utils/filtertime_s.c, parse HH:MM straight into seconds since 00:00 UTC and accept only that format. Make the end bound include the whole :59 second, i.e. raw < (end+1)*128. Check that 00:00-00:59 plus 01:00-01:59 equals 00:00-01:59 with cmp.

_From past Claude sessions here, Nov 8._
