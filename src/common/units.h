/* units.h - unit parser and converter.
 * Typed lowercase: "cm2" = cm², "mi/hr", "g/mol", "kj", "mhz".
 * Powers are digits after a unit (cm2), "^2" or "²".
 * A lone "m" is ambiguous (meter or molar M = mol/L): it is flagged and
 * resolved by unit_resolve_pair() / unit_resolve_m(). */
#ifndef CHEM_UNITS_H
#define CHEM_UNITS_H

#include "num.h"

#define NDIM 5          /* length, mass, time, amount, temperature */
#define UNIT_MAXT 4

typedef struct {
    uint8_t id;         /* index into unit table */
    int8_t pre;         /* power-of-ten prefix exponent (k = 3, m = -3), 0 = none */
    int8_t pow;         /* power, negative = denominator */
} uterm_t;

typedef struct {
    uint8_t n;
    uint8_t ambig;      /* bit i set: term i is a lone "m" (meter or molar) */
    uterm_t t[UNIT_MAXT];
} unit_t;

typedef struct {
    int8_t d[NDIM];
    fp_t f;             /* factor to SI base units */
} udim_t;

typedef struct {
    num_t n;
    unit_t u;
} qty_t;

/* unit ids that apps need by name */
enum { U_METER = 0, U_GRAM, U_SECOND, U_LITER, U_MOL, U_MOLAR, U_PARTICLE };

/* Parse units starting at s; returns chars used (0 = no unit). */
uint8_t unit_parse(const char *s, unit_t *u);
void unit_dim(const unit_t *u, udim_t *out);
bool unit_same_dim(const udim_t *a, const udim_t *b);
bool unit_is_empty(const unit_t *u);
/* value in `from` -> value in `to`; false if dimensions differ */
bool unit_convert(fp_t v, const unit_t *from, const unit_t *to, fp_t *out);
/* fix the ambiguous lone "m" terms: molar = true -> M, else meter */
void unit_resolve_m(unit_t *u, bool molar);
/* resolve ambiguous terms in both units so that from -> to is valid.
 * Prefers meter when both work. false if no combination works. */
bool unit_resolve_pair(unit_t *from, unit_t *to);
/* multiply/divide unit expressions (merges equal terms) */
void unit_mul(unit_t *a, const unit_t *b, int8_t sign);
/* number with optional unit: "2.45cm2", ".640m", "25g". Returns chars used. */
uint8_t qty_parse(const char *s, qty_t *q);
/* display: "mm²", "g/mol", "m/s²", "mol/L" */
uint8_t unit_str(const unit_t *u, char *buf);

#endif
