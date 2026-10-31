/* formula.h - chemical formula parser with auto-case.
 *   kcl -> KCl (74.55 g/mol, never K+C+I), fe(no3)2, (nh4)2so4,
 *   cuso4.5h2o (hydrate), fe3+ / so42- / no3- (ion charges).
 * Digits after letters are subscripts. Lowercase input is split into
 * element symbols by scoring every possible split (fewest symbols, common
 * elements, metals before nonmetals). Ambiguous strings co, cs, hf, no:
 * FP_COMPOUND gives the compound (CO) and sets alt_z to the element (Co);
 * FP_ELEMENT gives the element. */
#ifndef CHEM_FORMULA_H
#define CHEM_FORMULA_H

#include "fp.h"
#include "data.h"

#define F_MAXEL   12
#define F_MAXPART 8
#define F_DISP    40

enum { FP_COMPOUND = 0, FP_ELEMENT = 1 };

typedef struct {
    uint8_t z;
    uint16_t n;
} fel_t;

typedef struct {
    uint8_t start, len;   /* slice of disp */
    uint8_t mult;         /* subscript / group multiplier */
    bool group;           /* parenthesised group */
} fpart_t;

typedef struct {
    uint8_t nel;
    fel_t el[F_MAXEL];    /* total atoms per element, first-appearance order */
    int8_t charge;
    uint8_t coef;         /* leading coefficient ("2h2o"), 1 if none */
    uint8_t hydrate;      /* waters of hydration (CuSO4·5H2O -> 5) */
    uint8_t alt_z;        /* other reading is this element (co -> Co) */
    uint8_t nsym;         /* element symbols written (not counting repeats) */
    uint8_t npart;
    fpart_t part[F_MAXPART]; /* top-level parts before any hydrate dot */
    char disp[F_DISP];    /* canonical text, ASCII digits: "Fe(NO3)2", "CuSO4·5H2O" */
} formula_t;

/* Parse s[0..len). Returns true on success. */
bool formula_parse(const char *s, uint8_t len, uint8_t prefer, formula_t *f);
fp_t formula_mass(const formula_t *f);
/* atoms of element z in the formula */
uint16_t formula_count(const formula_t *f, uint8_t z);
/* "Fe(NO3)2", "Fe3+", "SO4 2-" in ASCII */
uint8_t formula_str(const formula_t *f, char *buf);
/* show-work line i (0..nel-1): "Fe 1x55.85=55.85" ; returns length */
uint8_t formula_work_line(const formula_t *f, uint8_t i, char *buf);
/* copy text, turning digits that follow a letter or ')' into subscripts */
uint8_t formula_pretty(const char *ascii, char *out);

#endif
