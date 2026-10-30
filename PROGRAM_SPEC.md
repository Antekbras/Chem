# TI-84 CE Chem Suite — Current Program Spec (from user photos)
Calc: TI-84 Plus CE, OS 5.8.5.0074, arTIfiCE. Course: college intro chem. No source exists; rebuild from this spec.
PRGM list today: ELEMENT, MOLE, NOMENC, ORBITAL, PHOTON, PTABLE, UNITCONV (all C/asm, "*" prefix).

## Shared UI (current)
- Header: 2-letter icon (Uc hv Or Nm Mo El) + title + subtitle, colored bar. Footer key hints.
- Form: GIVEN row (▲▼), UNIT row (◄►), value box with unit at right.
- Result: Given line, big ANSWER in sci notation w/ sig figs (+ plain decimal small), "Show work? [1]Yes [2]No", footer "1 work 2 menu ENTER again CLEAR menu".
- Show-work screen: numbered steps (e.g. molar mass breakdown, n=m/M, N=n·NA).
- PROBLEMS: (a) "ENTER opens entry" drops to OS home-screen prompt (e.g. `W=12`); afterwards unit/given can't be changed. (b) ELEMENT uses OS Input prompts for every field. Replace all with in-place editable fields.

## UNITCONV (Uc, "Sig figs · units · chemistry")
1 Sig Figs — count sig digits, show 10^x forms with units
2 Unit Changer — any units, compound units, chemistry (g H2O → mol)
3 Show work on/off ([graph] also toggles)

## PHOTON (hv)
Menu shows c = 2.998×10^8 m/s.
1 Find energy E=hν=hc/λ (given λ or ν) → J
2 Find frequency ν=c/λ → Hz
3 Find wavelength λ=c/ν → m and nm
4 Color: given λ|ν → spectrum region + visible color; or ALPHA+R/O/Y/G/B/I/V to look up a color
5 Speed of light: 2.998e8 (default), 3.00e8, 2.99792458e8 (used by all calcs)
λ units pm nm µm mm cm m km; ν units Hz kHz MHz GHz THz. Verified: λ=12 nm → E=1.66e-17 J, ν=2.50e16 Hz.

## ORBITAL (Or) — reference, 10 tabs; ◄►/ENTER prev/next, 1-9,0 jump, CLEAR quit
1 Quantum number rules (n, ℓ 0..n-1, mℓ -ℓ..+ℓ, ms ±1/2; s p d f = ℓ 0-3)
2 Number table (1s–4f: ℓ, mℓ values, #orbitals, ms)
3 Aufbau diagonal tree 1s–7p
4 Filling order table (sub, max, running sum, element that completes it: He…Og)
5 Capacity (s/p/d/f 2/6/10/14 e-, 1/3/5/7 orbitals; shell n: n² orbitals, 2n² e-)
6–9 s, p, d, f orbital shapes + box diagrams
10 Hund's rule (p¹–p⁶ boxes)

## NOMENC (Nm, "Chem nomenclature")
1 Formula → name (e.g. CO2 → CARBON DIOXIDE)
2 Name → formula
3 Tables  ← REMOVE
4 Atom charge (protons/neutrons/electrons) ← REMOVE
Covers ionic, covalent, acids, ions, hydrates, Roman numerals.
Entry box shows mode chips "1 A a LOCK"; alpha cycles 1/A/a; "BLANK+ENTER = MENU".

## MOLE (Mo, "Mole calculator"; footer NA = 6.022×10^23 /mol)
1 Moles → mass  2 Mass → moles  3 Particles (given particles|grams|moles → N and n)
Step 1 Substance: formula or g/mol; [Y=] common substances list; hydrate "CuSO4.5H2O"; hints "arrows switch 123/ABC/abc lock, ALPHA = one other-case letter".
Step 2 Amount: mass units g kg mg µg lb oz; "[.] types E: 1E22".
**BUG: "KCl" computes as K+C+I = 178.01 g/mol (lowercase l read as I). Correct 74.55. Formula parser must be case-correct; consider auto-case (first letter cap, following lowercase) so users needn't toggle case.**

## ELEMENT (El, "Isotope tools")
1 Find missing value (e.g. missing abundance / isotope mass) — label "107AG", blank = unknown
2 Weighted average atomic weight
Currently uses OS home-screen Input prompts: "ISOTOPE 1 (107AG):", "ABUNDANCE % (blank=?):". Results: warns if abundances ≠ 100% and normalizes.
Rebuild as single form with rows per isotope (mass, %), blank = solve.

## PTABLE — not photographed yet (periodic table viewer presumably).

## Proposed consolidation (Jarvis to confirm against tests)
- SIGFIG: home-screen-look calculator, sig-fig aware, units optional, full expressions + sci notation.
- CONVERT: unit changer + mole/mass/particles + molar mass (one "any→any" engine).
- ATOM: periodic table, isotopes/avg mass, orbitals/e- config, photon (light) calcs.
- NAMES: nomenclature (formula↔name only).
- BALANCE: new equation balancer.
