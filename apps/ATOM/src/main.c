/* ATOM - atoms, ions, isotopes and light, typed on one line:
 *   fe  cr  29  iron     config [Ar]4s2 3d6; [GRAPH] = element card
 *   fe3+  cu2+           ion config; small text = d electrons
 *   cl37  37cl  c-14     isotope: protons, neutrons, electrons
 *   12nm  12nm→hz  5.0E14hz→nm  3.6E-19j→nm  green
 * Field keys: [Y=] periodic table, [WINDOW] isotope form (average atomic
 * mass, blank = solve), [ZOOM] orbital reference, [TRACE] light and c. */
#include <string.h>
#include "atom.h"
#include "formula.h"
#include "state.h"

#define AST_VAR "ATOMxs"
#define AST_VERSION 1

atom_state_t ast;

static bool digit(char c) { return c >= '0' && c <= '9'; }

static uint16_t read_u(const char *s, uint8_t *len) {
    uint16_t v = 0;
    uint8_t i = 0;
    while (digit(s[i]) && i < 3) v = (uint16_t)(v * 10 + s[i++] - '0');
    *len = i;
    return v;
}

/* one element, optionally with a charge: "fe", "Fe3+", "iron" */
static uint8_t element_of(const char *s, uint8_t len, int8_t *charge) {
    formula_t f;
    uint8_t z = elem_find_name(s, len);
    *charge = 0;
    if (z) return z;
    if (formula_parse(s, len, FP_ELEMENT, &f) && f.nel == 1 && f.el[0].n == 1 && f.coef == 1 && !f.hydrate) {
        *charge = f.charge;
        return f.el[0].z;
    }
    return 0;
}

static void config_answer(uint8_t z, int8_t charge, char *out, char *small, work_t *w) {
    econfig_t c;
    char *p;
    econfig(z, charge, &c);
    econfig_str(&c, false, out);
    if (charge) {
        p = put_u(small, econfig_dcount(&c));
        put_s(p, " d e-");
    } else {
        put_mass(fmt(small, "%S ", z), z);
    }
    card(z, charge, w);
}

static void compute(const char *in, char *out, char *small, work_t *w) {
    uint8_t len = (uint8_t)strlen(in), n, z;
    int8_t q = 0;
    if (light(in, out, small, w)) return;
    if (digit(in[0])) {
        uint16_t v = read_u(in, &n);
        if (n == len) {                                   /* atomic number */
            if (v >= 1 && v <= NUM_ELEMENTS) { config_answer((uint8_t)v, 0, out, small, w); return; }
        } else if ((z = element_of(in + n, (uint8_t)(len - n), &q)) != 0 && v >= z) {
            isotope_card(z, v, q, out, small, w);         /* 37cl */
            return;
        }
    } else {
        /* trailing mass number: cl37, c-14 */
        uint8_t e = len;
        while (e && digit(in[e - 1])) e--;
        if (e && e < len && len - e <= 3) {
            uint8_t k;
            uint16_t a = read_u(in + e, &k);
            uint8_t sl = (uint8_t)(in[e - 1] == '-' ? e - 1 : e);
            z = element_of(in, sl, &q);
            if (z && a >= z && (a > 3 || sl != e || z == 1)) {
                isotope_card(z, a, q, out, small, w);
                return;
            }
        }
        z = element_of(in, len, &q);
        if (z) {
            econfig_t c;
            if (econfig(z, q, &c)) { config_answer(z, q, out, small, w); return; }
        }
    }
    app_echo(in, FP_ELEMENT, out, UI_OUT);
    strcpy(small, "?");
}

static void field(uint8_t n) {
    switch (n) {
    case 0: table_view(); break;
    case 1: iso_form(); break;
    case 2: orbital_ref(); break;
    case 3: light_screen(); break;
    }
}

int main(void) {
    static const char *const labels[5] = {"Table", "Isot", "Orbit", "Light", "Work"};
    static const app_def_t d = {"ATOM", "ATOMst", SMART_ALL, compute, field, 0, 0};
    if (!state_load(AST_VAR, &ast, sizeof ast, AST_VERSION)) {
        memset(&ast, 0, sizeof ast);
        ast.tz = 26;
    }
    app_labels(labels);
    app_run(&d);
    state_save(AST_VAR, &ast, sizeof ast, AST_VERSION);
    return 0;
}
