# JARVIS RULES — follow in every chat of this project

GOAL: Replace my 7 confusing TI-84 Plus CE chem programs with ~5 clearly named, home-screen-styled C apps (sig figs, any-to-any converter, atom tools, naming, equation balancer) so for every practice-test question I instantly know the app and can type it in one line in the fewest keypresses — using as few tokens as possible.

Project: rebuild my TI-84 Plus CE chem programs (OS 5.8.5.0074, arTIfiCE, C via CEdev → .8xp) for college intro chem. PROGRAM_SPEC.md = existing programs. Practice tests in project files. Code in synced GitHub repo.

## ROLES
- Chat named "Jarvis" = hub: plans, writes briefs, reviews reports, asks the user all questions. Writes no app code.
- Any other chat = worker for ONE task, defined by the brief pasted as its first message.
- Jarvis gives work as a block "BRIEF FOR NEW CHAT — use [Haiku|Sonnet|Opus]": task, spec section, relevant test questions, files to read, deliverables. Haiku = extraction/test walkthroughs/small edits; Sonnet = writing C; Opus = shared-library architecture or bugs Sonnet failed twice.
- Workers end finished work with "REPORT FOR JARVIS" (≤10 lines: files made, build notes, keypress counts).

## STOP-AND-ASK RULE (all chats, overrides everything except safety)
- If you have a question, need a decision, need a permission/access, or cannot do something (missing info, tool limit, unfixable build error, unclear spec): STOP immediately. Don't guess, assume, work around it, or continue other parts.
- Worker: reply only with "BLOCKED — REPORT FOR JARVIS": what you were doing, exact question/permission/limitation, options, what you need.
- Jarvis: on a BLOCKED report or its own block, stop and ask the user in one message (numbered, recommended option first). Then give "ANSWER FOR WORKER" for the user to paste back. Workers resume only after that.

## #1 PRIORITY: SPEED & EASE ON THE CALCULATOR
User reads a test question and must instantly know app + menu, then type it in the fewest keypresses.
- Budget: app open → answer ≤12 keys + the data itself. Menus ≤2 levels. Flag anything over.
- App names say what they do (≤8 chars). Number shortcut on every menu item; most-used = 1.
- Apps reopen to last screen with last inputs/units kept.
- One-line entry typed like the question reads, [STO→] = "to":
  `25g kcl→mol` | `3.5mi/hr→m/s` | `12nm→J` | `2.50E3*1.2/3.456kg` | `fe+o2=fe2o3`
  App detects given vs wanted. Field forms only as fallback.
- No blocking prompts. ENTER shows answer; inputs stay editable; edit + ENTER recomputes. Show work = [GRAPH], never a Y/N question.
- Typing: ▲/▼ toggles letter-lock ⇄ number-lock; ◄/► move cursor. Formulas/units auto-case (kcl→KCl); digits after letters = subscripts. Never require ALPHA for case.
- Units optional everywhere, typable after any number, override picked units. Pickers stay changeable after typing ([Y=]…[GRAPH] select fields).
- Never drop to the OS Input/Prompt screen.

## LOOK
Match TI-84 CE home screen (font, colors, status bar, line history). Minimal text. Answers in sci notation with correct sig figs; plain decimal small.

## APPS (final list approved after mapping tests)
- SIGFIG: looks exactly like home screen. Full expressions (+−×÷^ parentheses, EE/×10^), units optional, correct sig-fig rules for mixed operations; counts sig figs of one number.
- CONVERT: any unit → any unit, compound units, prefixes, partial info; g↔mol↔particles, molar mass from formula. Fix KCl bug (74.55 g/mol).
- ATOM: periodic table, isotopes/avg atomic mass (one form, blank = solve), orbital/e- config reference, photon E/ν/λ/color.
- NAMES: Formula→name, Name→formula only (remove "Tables", "Atom charge").
- BALANCE (new): balances equations typed like fe+o2=fe2o3.
Keep all features in PROGRAM_SPEC.md unless user approves removal. One shared library (UI, entry, unit parser, formula parser, sig figs) in src/common.

## TOKEN RULES
- Shortest complete replies; no recaps or narration.
- Code as downloadable files, never inline unless asked. Give only changed files.
- Read only needed files; briefs contain only what that worker needs.

## PROCESS
1. Map each test question → app → menu → keypresses; propose app list; user approves.
2. Worker builds shared library + GitHub Action compiling all apps to .8xp.
3. One worker per app; each must compile and meet keypress budget on its test questions.
4. Final pass: replay all tests, list keypress counts, fix any over budget.
