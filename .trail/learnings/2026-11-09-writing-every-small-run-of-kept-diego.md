---
title: Writing every small run of kept records separately made filters slower than before
author: Diego
date: 2026-11-09
standing: true
id: l-f0cde0fc
scope: repo
status: active
src: init-sessions
touches:
  - src/utils/gps_stream.h
---

## Watch out
- A filter that drops scattered records leaves runs of only a few records. One write() syscall per run was slower than the old buffered stdio, so small runs are now copied into one output buffer.
- In the code: In src/utils/gps_stream.h, runs under about 256 KB go into the 1 MiB output buffer and only large runs are written directly. When migrating another *_s utility, measure with interleaved categories and not only with long contiguous runs.

_From past Claude sessions here, Nov 8._
