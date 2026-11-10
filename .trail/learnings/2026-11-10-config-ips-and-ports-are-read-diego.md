---
title: "Config IPs and ports are read as decimal, zero padding allowed, never with inet_addr"
author: Diego
date: 2026-11-10
branch: v0.84-todo-fixes
cost: { minutes: 11, tokens: 104157 }
id: l-3136df06
scope: repo
status: active
src: agent
touches:
  - src/reader_network.c
  - tests/load/analyze.py
  - tests/load/run_load.sh
---

## Decided
IPs and ports in radar_definition are parsed as plain decimal, with or without leading zeros. inet_addr()/inet_aton() and strtol(..., 0) read a leading 0 as octal: "239.255.001.010" silently joined 239.255.1.8, "008"/"009" made the .conf fail, and port "05000" became 2560. The per-packet source check was already numeric (since 0.84); the bug was in the text-to-number step.
Hex ports with "0x" are still accepted (strtol base 0 used to allow them). Spaces around an IP are accepted, as inet_addr did. inet_addr's short forms ("10.1") and hex octets are now rejected on purpose: they are ambiguous in a hand-written config.
On SIGHUP reload, entries are compared field by field as numbers, so rewriting the same IP with another format does not log a radar as removed+added.

## Watch out
Any new code reading IPs/ports from the .conf must use the same helpers; one inet_addr() left behind brings the octal bug back for that path only.
Each .gps file carries a 2200-byte header: never `cat` several rotated files before running tests/load/analyze.py, it stops reading at the second header and reports large fake losses. Read each file with analyze.read_gps().

In the code: src/reader_network.c parse_ipv4(), cfg_ip(), cfg_port(), radar_field_equal(); scenario `zeropad` in tests/load/run_load.sh.
