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

## ATOM

Type on one line: `fe` `cr` `29` `iron` → electron config (small: symbol, mass) · `fe3+` → ion config (small: d electrons) ·
`cl37` `37cl` `c-14` → protons, neutrons, electrons · `12nm` → photon energy in J · `12nm→hz` `5.00E14hz→nm` `3.31E-19j→nm`
`500nm→kj/mol` (small: spectrum region or color) · `green` → its wavelength range. [GRAPH] = element card (mass, EN,
group/period/block, family, type, state, radii, isotope, full and noble-gas config, shells, valence and inner d/f e-,
unpaired e- with para/diamagnetic, orbital boxes, Lewis dots, ions) or the photon work.

| Key | Screen |
|---|---|
| [Y=] Table | periodic table; arrows move, 1 family 2 metal type 3 block 4 state, ENTER picks |
| [WINDOW] Isot | isotope form: mass and % rows + average atomic mass; blank = solve; warns and normalizes if % ≠ 100 |
| [ZOOM] Orbit | orbital reference, 10 pages; ◄ ► or ENTER, 1–9 0 jump |
| [TRACE] Light | spectrum regions and colors; 1–3 pick c = 2.998E8 / 3.00E8 / 2.99792458E8 |

Element radii and isotopes in `apps/ATOM/src/atomdata.c` come from `tools/gen_atomdata.py` (mendeleev package).
