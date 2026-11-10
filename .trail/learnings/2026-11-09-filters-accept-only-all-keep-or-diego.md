---
title: "Filters accept only all-keep or all-remove values, mixing is an error"
author: Diego
date: 2026-11-09
standing: true
id: l-b404143c
scope: repo
status: active
src: init-sessions
---

## Decided
- With several categories or sensors, keeping two and removing one of them makes no sense, since you could just keep one. The team chose to reject mixed signs at startup.
- In the code: In filtercat_s and filtersacsic_s (src/utils), the positive list means only those pass and the negative list means those are removed. Mixed signs must exit with an error at startup. Do not add precedence rules.

_From past Claude sessions here, Nov 8._
