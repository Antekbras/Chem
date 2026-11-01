/* convert.h - CONVERT engine (pure, host-tested).
 * One line, typed like the question reads, [STO→] = "to":
 *   2.45cm2→mm2   3.5mi/hr→m/s   mi/hr→m/s (no number = factor)
 *   25g kcl→mol   1.5mol h2o→g   5.23E24 co2→g   6.02E23→mol
 *   kcl  (molar mass)   25g kcl  (no "to": g→mol, mol→g, N→mol)
 *   250.0ml .640m→mol   .0500mol .250m→ml   4.00g naoh 500.0ml→m
 *   3.00m 25.0ml→150.0ml (dilution)   .100m→50.0ml .0500m
 *   pcl5→p (% P)   10.00g n2o4→g n   1.00g h2o→atoms h
 *   25g 58.44g/mol→mol (substance given as g/mol) */
#ifndef CHEM_CONVERT_H
#define CHEM_CONVERT_H

#include <stdbool.h>
#include "work.h"

void convert_compute(const char *in, char *out, char *small, work_t *w);

/* [Y=] common substances (MOLE's list) and [WINDOW] units */
#define NSUBST 17
extern const char *const subst[NSUBST];
#define NUNITLIST 36
extern const char *const unitlist[NUNITLIST];
/* text a picked item types: substances lowercase with a space before
 * them, units lowercase (molar M stays M) */
void convert_pick_text(const char *item, bool unit, char prev, char *out);
/* app field callback: [Y=] = 0, [WINDOW] = 1 */
void convert_field(uint8_t n);

#endif
