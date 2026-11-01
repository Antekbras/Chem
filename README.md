# Chem apps for the TI-84 Plus CE

Five C apps (CEdev → `.8xp`) for college intro chem: **SIGFIG**, **CONVERT**, **ATOM**, **NAMES**, **REACT**.
`PROGRAM_SPEC.md` describes the old programs, `JARVIS_RULES.md` the rules.

| Path | What |
|---|---|
| `src/common/` | shared library: `num` (sig figs), `expr` (calculator), `units`, `formula` (auto-case, molar mass), `data` (elements, configs, ions), `entry` (one-line editor, smart lock, key planner), `ui` (home-screen look), `state` (appvar), `app` (shared app loop) |
| `apps/<NAME>/` | one app each: `src/main.c` + CEdev `makefile` |
| `tests/` | host unit tests (`make test`), incl. every TEST_MAP key count |

Build: `make test` (gcc) · `make apps` (needs CEdev on `PATH`). CI builds all five `.8xp` files as the
`chem-apps-8xp` artifact. Send them with TI Connect CE; the apps use only OS routines (no clibs needed).

Keys: ▲/▼ letter-lock ⇄ number-lock · in number-lock MATH APPS PRGM x⁻¹ SIN COS TAN ^ x² LOG LN type
A–I, N, S (SIGFIG keeps ^ and x²) · [STO→] = → · ALPHA+key = one OS letter (X = ALPHA+[STO→], J = ALPHA+[,]) ·
2nd+[,] = E · [,] = space · ENTER computes, the line stays editable · [GRAPH] show work · 2nd+▲/▼ scroll ·
2nd+QUIT exits.

CONVERT: `2.45cm2→mm2` · `25g kcl→mol` · `kcl` (molar mass) · `5.23E24 co2→g` · `250.0ml .640m→mol` ·
`3.00m 25.0ml→150.0ml` (dilution) · `pcl5→p` (% P) · `10.00g n2o4→g n` · no number = conversion factor
(`mi/hr→m/s`) · no `→` = the usual next step (g→mol, mol→g, N→mol, mol→particles) · [Y=] common
substances · [WINDOW] units list.
