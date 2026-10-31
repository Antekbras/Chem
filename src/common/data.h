/* data.h - element table, electron configurations, polyatomic ions. */
#ifndef CHEM_DATA_H
#define CHEM_DATA_H

#include "fp.h"

#define NUM_ELEMENTS 118

enum { CAT_METAL = 0, CAT_NONMETAL = 1, CAT_METALLOID = 2, CAT_NOBLE = 3 };

typedef struct {
    char sym[3];
    uint8_t cat;          /* CAT_* */
    uint16_t en100;       /* Pauling electronegativity x100, 0 = none */
    uint32_t mass1000;    /* molar mass in mg/mol (g/mol x1000) */
    const char *name;     /* lowercase English name */
    int8_t ions[3];       /* common ion charges, most common first, 0 = end */
} element_t;

extern const element_t elements[NUM_ELEMENTS + 1]; /* index = Z, [0] unused */

fp_t elem_mass(uint8_t z);
/* Z for symbol (case-insensitive, "fe", "Fe", "FE"); 0 if none */
uint8_t elem_find(const char *sym, uint8_t len);
/* Z for English name ("iron"), 0 if none */
uint8_t elem_find_name(const char *name, uint8_t len);
bool elem_is_metal(uint8_t z);
/* rarity tier used by the formula auto-case: 0 common .. 3 exotic */
uint8_t elem_rarity(uint8_t z);

/* ---- electron configuration ---- */
#define NUM_SUBSHELLS 19
typedef struct {
    uint8_t occ[NUM_SUBSHELLS];  /* electrons per subshell, aufbau order */
    uint8_t core;                /* Z of noble-gas core used for shorthand (0 = none) */
    uint8_t ncore;               /* number of leading subshells inside the core */
    uint8_t electrons;
} econfig_t;

extern const uint8_t sub_n[NUM_SUBSHELLS];   /* principal quantum number */
extern const uint8_t sub_l[NUM_SUBSHELLS];   /* 0=s 1=p 2=d 3=f */

/* Ground-state config of atom/ion (charge +3 = cation). false if impossible. */
bool econfig(uint8_t z, int8_t charge, econfig_t *out);
/* "[Ar]4s1 3d5" (noble-gas shorthand when full is false). Returns length. */
uint8_t econfig_str(const econfig_t *c, bool full, char *buf);
/* d electrons outside the core (the d count asked on tests) */
uint8_t econfig_dcount(const econfig_t *c);

/* ---- polyatomic ions ---- */
typedef struct {
    const char *formula;   /* canonical case, e.g. "NO3", "C2H3O2" */
    int8_t charge;
    const char *name;      /* "nitrate" */
} polyion_t;

#define NUM_POLYIONS 34
extern const polyion_t polyions[NUM_POLYIONS];
/* index of polyatomic ion by canonical formula (exact case), -1 if none */
int polyion_find(const char *formula, uint8_t len);
int polyion_find_name(const char *name, uint8_t len);

#endif
