/* atom.h - ATOM app: configs, element card, isotopes, light, table, orbitals. */
#ifndef ATOM_H
#define ATOM_H

#include "app.h"
#include "data.h"
#include "num.h"
#include "units.h"

/* ---- atomdata.c (generated) ---- */
extern const uint16_t rad_emp[NUM_ELEMENTS + 1];
extern const uint16_t rad_cov[NUM_ELEMENTS + 1];
extern const uint8_t iso_n[NUM_ELEMENTS + 1];
extern const uint8_t state_bits[30];

/* ---- saved with the app (appvar ATOMxs) ---- */
#define ISO_ROWS 5
#define ISO_LEN 10
typedef struct {
    uint8_t c_sel;                     /* speed of light choice 0..2 */
    uint8_t view, tz;                  /* table view and selected Z */
    uint8_t tab;                       /* orbital reference tab */
    uint8_t irow, icol;                /* isotope form cursor */
    char iso[ISO_ROWS * 2 + 1][ISO_LEN + 1]; /* mass, % per row; then avg */
} atom_state_t;
extern atom_state_t ast;

/* ---- card.c: element facts ---- */
uint8_t el_group(uint8_t z);           /* 1..18, 0 = f-block series */
uint8_t el_period(uint8_t z);
char el_block(uint8_t z);              /* 's' 'p' 'd' 'f' */
uint8_t el_family(uint8_t z);          /* index into family_name */
extern const char *const family_name[];
extern const char *const state_name[];
extern const char *const type_name[];   /* by CAT_* */
uint8_t el_state(uint8_t z);
uint8_t unpaired(const econfig_t *c);
/* text helpers: append to p, return new end */
char *put_u(char *p, uint16_t v);
char *put_s(char *p, const char *s);
/* tiny printf: %u %s %c, %S = symbol of Z, %q = charge ("3+") */
char *fmt(char *p, const char *f, ...);
void wf(work_t *w, const char *f, ...);
/* "55.845", radioactive "[98]" */
char *put_mass(char *p, uint8_t z);
/* element / ion card in the work lines */
void card(uint8_t z, int8_t charge, work_t *w);
/* isotope "cl37" / "37cl": out "17p 20n 17e" */
void isotope_card(uint8_t z, uint16_t a, int8_t charge, char *out, char *small, work_t *w);

/* ---- light.c ---- */
extern const char *const c_text[3];
/* "12nm", "12nm>j", "5.0E14hz>nm", "3.0E-19j>nm", "green": true if handled */
bool light(const char *in, char *out, char *small, work_t *w);
void light_screen(void);

/* ---- iso.c ---- */
void iso_form(void);

/* ---- table.c ---- */
void table_view(void);

/* ---- orbital.c ---- */
void orbital_ref(void);

/* ---- small pixel helpers (table.c) ---- */
void px_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
uint8_t wait_key(void);

#endif
