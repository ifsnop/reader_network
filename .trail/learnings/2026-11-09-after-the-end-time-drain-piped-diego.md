---
title: "After the end time, drain piped input instead of exiting early"
author: Diego
date: 2026-11-09
standing: true
id: l-beb95b2a
scope: repo
status: active
src: init-sessions
---

## Decided
- Exiting early on stdin would kill the upstream process (bzcat or the previous filter) with SIGPIPE, which is bad with pipefail or tee. The decision is to cut straight away for regular files and drain unparsed for stdin.
- In the code: In gps_stream.h (gs_stop and the drain path), keep the split: regular file means stop reading and exit. stdin means read and discard in big blocks without parsing. Flush the pending output run before draining, because draining reuses the buffer.

_From past Claude sessions here, Nov 8._
