---
title: "systemctl reload validates the config first, since signals can't report failure"
author: Diego
date: 2026-11-09
standing: true
id: l-9023d6d3
scope: repo
status: active
src: init-sessions
---

## Tried and ruled out
- A plain kill -HUP is asynchronous, so systemctl reload said OK even when the process rejected the .conf. Type=notify-reload was dropped because it needs a newer systemd and still cannot report a failed reload.
- In the code: systemd/reader_network@.service runs two ExecReload lines. The first is `reader_network64 -t <conf>`, which validates and exits 0 or 1. The second is `kill -HUP`. Deploy the new binary together with the new unit, because a binary without -t makes every reload fail.

_From past Claude sessions here, Oct 13._
