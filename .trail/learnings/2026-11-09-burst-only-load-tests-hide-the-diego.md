---
title: Burst-only load tests hide the per-loop cost of the capture loop
author: Diego
date: 2026-11-09
standing: true
id: l-3777624a
scope: repo
status: active
src: init-sessions
touches:
  - tests/load/run_load.sh
---

## Watch out
- The sender fired all 150 streams at once in bursts, so each wait returned many ready sockets and few loops ran. Spreading the same traffic evenly cost the old binary nine times more CPU, almost all in the kernel and not in the memset.
- In the code: When measuring reader_network changes, use tests/load/run_load.sh with the `spread` and `spreaddecode` scenarios (sender.py --spread) as well as the burst ones. Do not conclude that a per-loop change is free from the burst scenarios alone.

_From past Claude sessions here, Oct 13._
