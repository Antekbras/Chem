/* ionic.h - what a compound does in water: its ions, solubility and
 * electrolyte class (REACT predictions, NAMES info line).
 *   NaCl -> Na+ Cl-, Fe(NO3)2 -> Fe2+ 2NO3-, (NH4)2SO4, H2SO4 (acid),
 *   BaSO4 insoluble, HF weak acid, NH3 weak base, C12H22O11 nonelectrolyte.
 * Solubility uses the usual intro-chem rules; the cation charge comes from
 * the anion (CuNO3 -> Cu+, Fe2O3 -> Fe3+). */
#ifndef CHEM_IONIC_H
#define CHEM_IONIC_H

#include "formula.h"

enum { IK_NONE = 0, IK_IONIC, IK_ACID };          /* ionic_t.kind */
enum {                                             /* ionic_class() */
    EL_NONELEC = 0, EL_STRONG_ACID, EL_WEAK_ACID, EL_STRONG_BASE,
    EL_WEAK_BASE, EL_SALT, EL_INSOLUBLE
};

typedef struct {
    uint8_t kind;          /* IK_* ; IK_NONE = molecular or element */
    char cat[7], an[7];    /* "Na" "NH4" "H" ; "SO4" "Cl" "OH" */
    uint8_t zc, za;        /* element Z of a one-atom ion, 0 for polyatomic */
    int8_t qc, qa;         /* +2, -1 */
    uint8_t nc, na;        /* ions per formula unit */
} ionic_t;

/* split a compound into its ions; false (kind IK_NONE) if molecular */
bool ionic_split(const formula_t *f, ionic_t *o);
/* water-soluble (acids always; molecular compounds: true) */
bool ionic_soluble(const ionic_t *o);
/* EL_* class of the compound */
uint8_t ionic_class(const formula_t *f, const ionic_t *o);
/* "Fe2+", "NO3-", "SO4 2-": ion text with charge; returns length */
uint8_t ion_str(const char *sym, int8_t q, char *buf);
/* group 1 (Li..Cs) */
bool ion_alkali(uint8_t z);
/* Ca Sr Ba (and Mg when `mg`) */
bool ion_heavy2(uint8_t z, bool mg);

#endif
