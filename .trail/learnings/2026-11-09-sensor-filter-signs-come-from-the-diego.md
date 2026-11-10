---
title: "Sensor filter signs come from the text, so -0 works for SAC 0"
author: Diego
date: 2026-11-09
standing: true
id: l-bb51b1bf
scope: repo
status: active
src: init-sessions
---

## Watch out
- Reading the sign with atoi made "-0 -118" and "0 -118" remove every sensor with SIC 118. A pair with one sign also acted as a wildcard, removing a whole SAC.
- In the code: In filtersacsic_s, read the sign from the argument string and not from atoi's value. Both numbers of a pair need the same sign. "0 -118" is accepted as a synonym of "-0 -118". Other sign mixes ("-20 5", "20 -5") are errors.

_From past Claude sessions here, Nov 8._
