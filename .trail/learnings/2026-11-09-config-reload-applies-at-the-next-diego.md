---
title: "Config reload applies at the next file rotation, only for sensors and FTP"
author: Diego
date: 2026-11-09
standing: true
id: l-5f267ef7
scope: repo
status: active
src: init-sessions
touches:
  - src/reader_network.c
---

## Decided
- Applying changes mid-file would leave a recording with mixed sensor sets. So the file is read and validated when SIGHUP arrives, but the change waits for the next UTC rotation.
- In the code: In src/reader_network.c, SIGHUP only sets reload_requested. read_pending_config() validates at once, and apply_pending_config() runs at rotation before the fork(). Only radar_definition and dest_ftp_uri are reloaded. Other changed keys log a restart-required warning. SIGHUP outside mode_continuous is ignored and no longer kills the process.

_From past Claude sessions here, Oct 13._
