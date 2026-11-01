/* REACT - balance, predict, stoichiometry.
 *   fe+o2→fe2o3                       balanced equation, type, coefficient sum
 *   cu+hcl   c3h8+o2   agno3+nacl     predicted products (or "no reaction")
 *   12.0mol n2+30.0mol h2→nh3         limiting reagent, leftover, yield
 *   163kg ch4+o2→co2+h2o→mol o2       amount of any species
 *   100.0g c7h6o+c2h4o→c9h8o+h2o→100.0g c9h8o   percent yield
 *   *.835 (Ans*.835)                  home-screen math on the last answer
 * [GRAPH]: atom counts per side, moles, limiting-reagent steps. */
#include <string.h>
#include "app.h"
#include "expr.h"
#include "formula.h"
#include "ionic.h"

#define MAXSP 8
/* separate functions: smaller code on the calculator than one inlined blob */
#define NOINLINE __attribute__((noinline))
#define NA ((fp_t)6.02214076e23L)

typedef struct {
    formula_t f;
    qty_t amt;          /* typed amount (given) */
    bool given;
    int32_t coef;
} sp_t;

static sp_t sp[MAXSP];
static uint8_t nsp, nre;            /* species, of which reactants */
static char *obeg, *op, *oend;     /* answer being built */
static work_t *wk;
static char tmp[64];

/* Ans survives closing the app */
static struct { qty_t ans; bool has_ans; } mem;

/* ---------- output helpers ---------- */

static void row(const char *s) {
    size_t n = strlen(s);
    if (op != obeg) {
        if (op >= oend) return;
        *op++ = '\n';
    }
    if (n > (size_t)(oend - op)) n = (size_t)(oend - op);
    memcpy(op, s, n);
    op += n;
    *op = 0;
}

static char *put(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }

static char *put_int(char *p, int32_t v) {
    char d[12];
    int8_t q = 0;
    if (v < 0) { *p++ = CH_NEG; v = -v; }
    do { d[q++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (q) *p++ = d[--q];
    *p = 0;
    return p;
}

/* formula with subscripts and charge */
static char *put_f(char *p, const formula_t *f) {
    char a[48];
    formula_str(f, a);
    return p + formula_pretty(a, p);
}

static void wline(const char *s) {
    /* wrap long show-work text onto more lines */
    while (strlen(s) > WORK_COLS) {
        char l[WORK_COLS + 1];
        memcpy(l, s, WORK_COLS);
        l[WORK_COLS] = 0;
        work_add(wk, l, NULL, NULL, NULL);
        s += WORK_COLS;
    }
    work_add(wk, s, NULL, NULL, NULL);
}

/* ---------- parsing ---------- */

static bool is_sep(char c) { return c == '+' || c == ' ' || c == 0; }

/* lone element text like "Co" "H2" "Pb" is parsed as the element */
static bool lone_element(const char *s) {
    if (!(s[0] >= 'A' && s[0] <= 'Z')) return false;
    s += (s[1] >= 'a' && s[1] <= 'z') ? 2 : 1;
    while (*s >= '0' && *s <= '9') s++;
    return !*s;
}

static bool add(const char *s, uint8_t len) {
    sp_t *x;
    if (nsp >= MAXSP) return false;
    x = &sp[nsp];
    memset(x, 0, sizeof *x);
    if (!formula_parse(s, len, (len < 6 && lone_element(s)) ? FP_ELEMENT : FP_COMPOUND, &x->f))
        return false;
    x->f.coef = 1;
    nsp++;
    return true;
}

/* proper-case text made by the predictor */
static bool addc(const char *s) { return add(s, (uint8_t)strlen(s)); }

/* "12.0mol n2", ".100m 25.0ml hcl", "h2o": amounts, then the formula */
NOINLINE static bool term(const char *s, uint8_t len, qty_t *amt, bool *given, const char **fs, uint8_t *fl) {
    const char *end = s + len, *last;
    *given = false;
    while (len && *s == ' ') { s++; len--; }
    while (end > s && end[-1] == ' ') end--;
    if (end <= s) return false;
    last = end;
    while (last > s && last[-1] != ' ') last--;
    *fs = last;
    *fl = (uint8_t)(end - last);
    while (s < last) {
        qty_t q;
        const char *tok = s;
        uint8_t n;
        while (*tok == ' ') tok++;
        if (tok >= last) break;
        n = qty_parse(tok, &q);
        if (!n || (tok[n] != ' ')) return false;
        if (!*given) { *amt = q; *given = true; }
        else { amt->n = num_mul(amt->n, q.n, false); unit_mul(&amt->u, &q.u, 1); }
        s = tok + n;
    }
    return true;
}

/* one side "a+b+c"; a '+' that ends an ion (fe3++o2) is part of it */
NOINLINE static bool side(const char *s, uint8_t len) {
    uint8_t i = 0, st = 0;
    for (;;) {
        if (i == len || (s[i] == '+' && !(i && !is_sep(s[i - 1]) && (i + 1 == len || is_sep(s[i + 1]))))) {
            qty_t q;
            bool g;
            const char *fs;
            uint8_t fl;
            if (!term(s + st, (uint8_t)(i - st), &q, &g, &fs, &fl) || !add(fs, fl)) return false;
            if (g) {
                sp[nsp - 1].amt = q;
                sp[nsp - 1].given = true;
            }
            if (i == len) return true;
            st = (uint8_t)(i + 1);
        }
        i++;
    }
}

/* ---------- balancing ---------- */

static int32_t gcd32(int32_t a, int32_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { int32_t t = a % b; a = b; b = t; }
    return a;
}

static uint8_t els[24], nels;       /* elements in order of appearance */

static void list_elements(void) {
    uint8_t j, k, m;
    nels = 0;
    for (j = 0; j < nsp; j++)
        for (k = 0; k < sp[j].f.nel; k++) {
            uint8_t z = sp[j].f.el[k].z;
            for (m = 0; m < nels && els[m] != z; m++) ;
            if (m == nels && nels < sizeof els) els[nels++] = z;
        }
}

NOINLINE static bool balance(void) {
    static int32_t a[25][MAXSP];
    uint8_t m = 0, r = 0, c, i, k, piv[25];
    bool charged = false;
    int32_t L = 1, g = 0;
    int32_t x[MAXSP];
    list_elements();
    for (i = 0; i < nels; i++, m++)
        for (c = 0; c < nsp; c++)
            a[m][c] = (int32_t)formula_count(&sp[c].f, els[i]) * (c < nre ? 1 : -1);
    for (c = 0; c < nsp; c++) charged |= sp[c].f.charge != 0;
    if (charged) {
        for (c = 0; c < nsp; c++) a[m][c] = sp[c].f.charge * (c < nre ? 1 : -1);
        m++;
    }
    /* integer row reduction */
    for (c = 0; c < nsp && r < m; c++) {
        for (i = r; i < m && !a[i][c]; i++) ;
        if (i == m) continue;
        for (k = 0; k < nsp; k++) { int32_t t = a[i][k]; a[i][k] = a[r][k]; a[r][k] = t; }
        for (i = 0; i < m; i++) {
            int32_t p = a[r][c], q = a[i][c], h = 0;
            if (i == r || !q) continue;
            for (k = 0; k < nsp; k++) {
                a[i][k] = a[i][k] * p - a[r][k] * q;
                h = gcd32(h, a[i][k]);
            }
            if (h > 1) for (k = 0; k < nsp; k++) a[i][k] /= h;
        }
        piv[r++] = c;
    }
    if (r >= nsp) return false;
    for (i = 0; i < r; i++) L = L / gcd32(L, a[i][piv[i]]) * (a[i][piv[i]] < 0 ? -a[i][piv[i]] : a[i][piv[i]]);
    for (c = 0; c < nsp; c++) x[c] = L;              /* free species = L */
    for (i = 0; i < r; i++) {
        int32_t s = 0;
        for (k = 0; k < nsp; k++) {
            bool pv = false;
            uint8_t t;
            for (t = 0; t < r; t++) pv |= piv[t] == k;
            if (!pv) s += a[i][k] * L;
        }
        x[piv[i]] = -s / a[i][piv[i]];
    }
    for (c = 0; c < nsp; c++) {
        if (x[c] <= 0 || x[c] > 1000000L) return false;
        g = gcd32(g, x[c]);
    }
    for (c = 0; c < nsp; c++) sp[c].coef = x[c] / g;
    return true;
}

/* ---------- chemistry facts ---------- */

static bool is_f(const formula_t *f, const char *s) { return !strcmp(f->disp, s) && !f->charge; }
static bool elem(const formula_t *f) { return f->nel == 1 && !f->charge; }

/* activity series, most active first; H is the dividing line */
static const uint8_t series[] = {3, 19, 56, 38, 20, 11, 12, 13, 25, 30, 24, 26, 48, 27, 28, 50, 82,
                                 1, 29, 47, 80, 78, 79};
static uint8_t act(uint8_t z) {
    uint8_t i;
    for (i = 0; i < sizeof series; i++) if (series[i] == z) return i;
    return 0xFF;
}

/* halogens F Cl Br I: lower = more active */
static uint8_t hal(uint8_t z) {
    return z == 9 ? 1 : z == 17 ? 2 : z == 35 ? 3 : z == 53 ? 4 : 0;
}

/* element's natural formula: H2 N2 O2 F2 Cl2 Br2 I2 P4 S8, else symbol */
static char *put_elem(char *p, uint8_t z) {
    p = put(p, elements[z].sym);
    if (z == 1 || z == 7 || z == 8 || z == 9 || z == 17 || z == 35 || z == 53) *p++ = '2';
    else if (z == 15) *p++ = '4';
    else if (z == 16) *p++ = '8';
    *p = 0;
    return p;
}

static bool poly(const char *s) {
    uint8_t up = 0;
    for (; *s; s++) {
        if (*s >= '0' && *s <= '9') return true;
        if (*s >= 'A' && *s <= 'Z') up++;
    }
    return up > 1;
}

static char *put_ion(char *p, const char *s, uint8_t n) {
    bool par = n > 1 && poly(s);
    if (par) *p++ = '(';
    p = put(p, s);
    if (par) *p++ = ')';
    if (n > 1) p = put_int(p, n);
    *p = 0;
    return p;
}

/* neutral compound of cation (charge qc) and anion (charge qa < 0) */
static void salt(char *b, const char *cat, int8_t qc, const char *an, int8_t qa) {
    int32_t g = gcd32(qc, qa);
    char *p = b;
    if (!strcmp(cat, "H") && !strcmp(an, "OH")) { strcpy(b, "H2O"); return; }
    p = put_ion(p, cat, (uint8_t)(-qa / g));
    put_ion(p, an, (uint8_t)(qc / g));
}

/* charge a metal takes: displacement (Fe2+, Sn2+) or combination */
static int8_t mcharge(uint8_t z, bool displace) {
    if (displace && (z == 26 || z == 50)) return 2;
    return elements[z].ions[0] > 0 ? elements[z].ions[0] : 0;
}

static const char *why;             /* "no reaction" reason */
static char whyb[26];

static bool no_rxn(const char *a, const char *b) {
    char *p = put(whyb, a);
    p = put(p, " below ");
    put(p, b);
    why = whyb;
    return false;
}

/* fill the products of sp[0..nre); false = no reaction (why set) or unknown */
NOINLINE static bool predict(void) {
    formula_t *A = &sp[0].f, *B = nre > 1 ? &sp[1].f : 0;
    ionic_t ia, ib;
    char b[24];
    why = 0;
    if (nre == 2 && (is_f(B, "O2") || is_f(A, "O2"))) {
        formula_t *o = is_f(A, "O2") ? B : A;
        uint8_t k, other = 0;
        for (k = 0; k < o->nel; k++) other += o->el[k].z != 1 && o->el[k].z != 6 && o->el[k].z != 8;
        if (formula_count(o, 6) && formula_count(o, 1) && !other)
            return addc("CO2") && addc("H2O");
    }
    if (nre == 2) {
        bool ea = elem(A), eb = elem(B);
        formula_t *E = ea ? A : B, *C = ea ? B : A;
        uint8_t ze, zc;
        if (ea && eb) {
            uint8_t za = A->el[0].z, zb = B->el[0].z, zm, zx;
            if (elem_is_metal(za) != elem_is_metal(zb)) {
                zm = elem_is_metal(za) ? za : zb;
                zx = elem_is_metal(za) ? zb : za;
                if (!mcharge(zm, false) || elements[zx].ions[0] >= 0) return false;
                salt(b, elements[zm].sym, mcharge(zm, false), elements[zx].sym, elements[zx].ions[0]);
                return addc(b);
            }
            {
                static const char *const pairs[] = {
                    "H\x08" "H2O", "H\x07" "NH3", "H\x09" "HF", "H\x11" "HCl", "H\x23" "HBr",
                    "H\x35" "HI", "C\x08" "CO2", "S\x08" "SO2", "P\x08" "P4O10", "N\x08" "NO2"};
                uint8_t i;
                for (i = 0; i < sizeof pairs / sizeof pairs[0]; i++) {
                    uint8_t z1 = elem_find(pairs[i], 1), z2 = (uint8_t)pairs[i][1];
                    if ((za == z1 && zb == z2) || (zb == z1 && za == z2)) return addc(pairs[i] + 2);
                }
            }
            return false;
        }
        if (!ea && !eb) goto two_compounds;
        ze = E->el[0].z;
        ionic_split(C, &ia);
        if (elem_is_metal(ze)) {
            uint8_t q = (uint8_t)mcharge(ze, true);
            if (!q) return false;
            if (is_f(C, "H2O")) {
                if (act(ze) > 5) return no_rxn(elements[ze].sym, "Ca");
                salt(b, elements[ze].sym, (int8_t)q, "OH", -1);
                return addc(b) && addc("H2");
            }
            if (ia.kind == IK_NONE) return false;
            zc = ia.kind == IK_ACID ? 1 : ia.zc;
            if (act(ze) == 0xFF || act(zc) == 0xFF) return false;
            if (act(ze) > act(zc)) {
                char sb[4];
                strcpy(sb, ia.kind == IK_ACID ? "H2" : elements[zc].sym);
                return no_rxn(elements[ze].sym, sb);
            }
            salt(b, elements[ze].sym, (int8_t)q, ia.an, ia.qa);
            if (!addc(b)) return false;
            put_elem(b, zc);
            return addc(b);
        }
        if (hal(ze) && ia.kind != IK_NONE && hal(ia.za)) {
            if (hal(ze) > hal(ia.za)) {
                put_elem(b, ia.za);
                return no_rxn(E->disp, b);
            }
            salt(b, ia.cat, ia.qc, elements[ze].sym, -1);
            if (!addc(b)) return false;
            put_elem(b, ia.za);
            return addc(b);
        }
        return false;
    two_compounds:
        if (!ionic_split(A, &ia) || !ionic_split(B, &ib)) return false;
        if (!strcmp(ia.cat, ib.cat) || !strcmp(ia.an, ib.an)) {
            why = "same ions";
            return false;
        }
        {
            /* swap partners; water, gases and solids drive the reaction */
            bool drive = false;
            uint8_t s;
            for (s = 0; s < 2; s++) {
                const ionic_t *c = s ? &ib : &ia, *n = s ? &ia : &ib;
                const char *cat = c->cat, *an = n->an;
                bool h = !strcmp(cat, "H");
                if (h && !strcmp(an, "CO3")) {
                    if (!addc("H2O") || !addc("CO2")) return false;
                    drive = true;
                } else if (h && !strcmp(an, "SO3")) {
                    if (!addc("H2O") || !addc("SO2")) return false;
                    drive = true;
                } else if (!strcmp(cat, "NH4") && !strcmp(an, "OH")) {
                    if (!addc("NH3") || !addc("H2O")) return false;
                    drive = true;
                } else {
                    ionic_t pi;
                    salt(b, cat, c->qc, an, n->qa);
                    if (!addc(b)) return false;
                    if (h) {   /* acid formed: water, gas (H2S), or weak acid from a strong one */
                        ionic_split(&sp[nsp - 1].f, &pi);
                        if (ionic_class(&sp[nsp - 1].f, &pi) != EL_STRONG_ACID) drive = true;
                    } else if (ionic_split(&sp[nsp - 1].f, &pi) && !ionic_soluble(&pi)) drive = true;
                }
            }
            if (!drive) { why = "all ions stay in water"; return false; }
            return true;
        }
    }
    /* one reactant: decomposition */
    if (nre != 1) return false;
    if (is_f(A, "H2O2")) return addc("H2O") && addc("O2");
    if (is_f(A, "H2CO3")) return addc("H2O") && addc("CO2");
    if (ionic_split(A, &ia) && ia.kind == IK_IONIC) {
        if (!strcmp(ia.an, "ClO3")) { salt(b, ia.cat, ia.qc, "Cl", -1); return addc(b) && addc("O2"); }
        if (!strcmp(ia.an, "CO3")) { salt(b, ia.cat, ia.qc, "O", -2); return addc(b) && addc("CO2"); }
        if (!strcmp(ia.an, "OH") && ia.zc) { salt(b, ia.cat, ia.qc, "O", -2); return addc(b) && addc("H2O"); }
    }
    if (A->nel == 2 && !A->charge) {        /* binary compound -> elements */
        put_elem(b, A->el[0].z);
        if (!addc(b)) return false;
        put_elem(b, A->el[1].z);
        return addc(b);
    }
    return false;
}

/* reaction type from the species */
static const char *rtype(void) {
    uint8_t j, npr = (uint8_t)(nsp - nre), er = 0, ep = 0;
    bool o2 = false, co2 = false, h2o = false, acid = false, base = false;
    for (j = 0; j < nsp; j++) {
        ionic_t io;
        const formula_t *f = &sp[j].f;
        if (j < nre) {
            o2 |= is_f(f, "O2");
            er += elem(f);
            ionic_split(f, &io);
            acid |= io.kind == IK_ACID;
            base |= io.kind == IK_IONIC && !strcmp(io.an, "OH");
        } else {
            co2 |= is_f(f, "CO2");
            h2o |= is_f(f, "H2O");
            ep += elem(f);
        }
    }
    for (j = 0; j < nsp; j++) if (sp[j].f.charge) return 0;
    if (o2 && co2 && h2o) return "combustion";
    if (nre >= 2 && npr == 1) return "combination";
    if (nre == 1 && npr >= 2) return "decomposition";
    if (nre == 2 && npr >= 2) {
        if (er == 1 && ep == 1) return "single displacement";
        if (!er && !ep) return acid && base && h2o ? "neutralization" : "double displacement";
    }
    return 0;
}

/* ions of species j if it is a dissolved strong electrolyte */
static bool aq_ions(uint8_t j, ionic_t *io) {
    uint8_t k;
    if (!ionic_split(&sp[j].f, io)) return false;
    k = ionic_class(&sp[j].f, io);
    return k == EL_STRONG_ACID || k == EL_STRONG_BASE || k == EL_SALT;
}

/* "spectators Na+ NO3-" and "ppt AgCl" rows (double/single displacement) */
NOINLINE static void ion_rows(bool dd) {
    char line[48], *p = put(line, "spectators");
    uint8_t j, k, n = 0;
    ionic_t a, b;
    for (j = 0; j < nre; j++) {
        if (!aq_ions(j, &a)) continue;
        for (k = 0; k < 2; k++) {
            const char *s = k ? a.an : a.cat;
            int8_t q = k ? a.qa : a.qc;
            uint8_t m, t;
            bool both = false;
            for (m = nre; m < nsp && !both; m++)
                if (aq_ions(m, &b))
                    for (t = 0; t < 2; t++)
                        both |= !strcmp(t ? b.an : b.cat, s) && (t ? b.qa : b.qc) == q;
            if (!both || strstr(line, s)) continue;
            *p++ = ' ';
            p += ion_str(s, q, p);
            n++;
        }
    }
    if (n) { row(line); work_add(wk, line, NULL, NULL, NULL); }
    if (!dd) return;
    for (j = nre; j < nsp; j++)
        if (ionic_split(&sp[j].f, &a) && !ionic_soluble(&a)) {
            p = put(line, "ppt ");
            put_f(p, &sp[j].f);
            row(line);
        }
}

/* "2Fe+3Cl2→2FeCl3" (pretty) ; returns position of the arrow */
NOINLINE static uint8_t equation(char *b) {
    char *p = b, *arrow = b;
    uint8_t j;
    for (j = 0; j < nsp; j++) {
        if (j == nre) { arrow = p; *p++ = CH_STO; }
        else if (j) *p++ = '+';
        if (sp[j].coef != 1) p = put_int(p, sp[j].coef);
        p = put_f(p, &sp[j].f);
    }
    *p = 0;
    return (uint8_t)(arrow - b);
}

NOINLINE static void eq_rows(void) {
    char b[100];
    uint8_t ar = equation(b);
    if (strlen(b) <= UI_COLS) { row(b); }
    else {
        char l[UI_COLS + 1];
        uint8_t n = ar > UI_COLS ? UI_COLS : ar;
        memcpy(l, b, n); l[n] = 0;
        row(l);
        row(b + ar);
    }
}

NOINLINE static void atom_work(void) {
    uint8_t i, j;
    for (i = 0; i < nels; i++) {
        int32_t l = 0, r = 0;
        char *p = put(tmp, elements[els[i]].sym);
        for (j = 0; j < nsp; j++) {
            int32_t n = sp[j].coef * formula_count(&sp[j].f, els[i]);
            if (j < nre) l += n; else r += n;
        }
        if (!tmp[1]) *p++ = ' ';
        *p++ = ' ';
        p = put_int(p, l);
        p = put(p, " = ");
        put_int(p, r);
        work_add(wk, tmp, NULL, NULL, NULL);
    }
}

/* ---------- stoichiometry ---------- */

static unit_t u_mol, u_g;

/* typed amount -> moles; kind 0 amount, 1 mass, 2 particles */
NOINLINE static bool to_mol(const sp_t *x, num_t *mol, uint8_t *kind) {
    fp_t v;
    unit_t u = x->amt.u;
    int sf = x->amt.n.sf;
    unit_resolve_m(&u, true);
    if (unit_is_empty(&u)) {
        *kind = 2;
        *mol = num_div(x->amt.n, num_exact(NA), false);
    } else if (unit_convert(x->amt.n.v, &u, &u_mol, &v)) {
        *kind = 0;
        *mol = num_sig(v, sf);
    } else if (unit_convert(x->amt.n.v, &u, &u_g, &v)) {
        *kind = 1;
        *mol = num_div(num_sig(v, sf), num_exact(formula_mass(&x->f)), false);
    } else return false;
    return true;
}

/* moles -> the number in unit `u` (empty = particles) */
NOINLINE static bool from_mol(num_t mol, const formula_t *f, const unit_t *u, num_t *out) {
    fp_t v;
    if (unit_is_empty(u)) { *out = num_mul(mol, num_exact(NA), false); return true; }
    if (unit_convert(mol.v, &u_mol, u, &v) ||
        unit_convert(mol.v * formula_mass(f), &u_g, u, &v)) {
        *out = num_sig(v, mol.sf);
        return true;
    }
    return false;
}

/* "2.03ᴇ4 mol O₂" ; alt = the other number form */
NOINLINE static void amount_text(num_t n, const unit_t *u, const formula_t *f, const char *tail, char *b, char *alt) {
    char m[32], us[16];
    char *p;
    num_fmt_answer(num_round(n), m, alt);
    p = put(put(b, m), " ");
    if (!unit_is_empty(u)) { unit_str(u, us); p = put(put(p, us), " "); }
    p = put_f(p, f);
    if (tail) put(p, tail);
}

/* the unit a given amount is shown in: as typed if one plain unit */
static const unit_t *show_unit(const sp_t *x, uint8_t kind) {
    unit_t u = x->amt.u;
    unit_resolve_m(&u, true);
    if (kind == 2 || (u.n == 1 && u.t[0].pow == 1 && !u.ambig)) return &x->amt.u;
    return kind == 1 ? &u_g : &u_mol;
}

static int8_t find(const char *s, uint8_t len) {
    formula_t f;
    uint8_t j, k;
    for (k = 0; k < 2; k++)
        if (formula_parse(s, len, k, &f))
            for (j = 0; j < nsp; j++) if (!strcmp(sp[j].f.disp, f.disp)) return (int8_t)j;
    return -1;
}

static void wnum(const char *a, num_t n, const char *b) {
    char x[24];
    num_fmt_plain(num_round(n), x);
    work_add(wk, a, x, b, NULL);
}

NOINLINE static bool stoich(const char *want, uint8_t wlen, char *small) {
    static num_t mol[MAXSP];
    num_t xi;
    uint8_t kind[MAXSP], j, lim = 0xFF, ng_re = 0;
    bool tie = false;
    char b[64], alt[24];
    const unit_t *u;
    xi.v = 0;
    for (j = 0; j < nsp; j++) {
        if (!sp[j].given) continue;
        if (!to_mol(&sp[j], &mol[j], &kind[j])) return false;
        if (j < nre) ng_re++;
        put(put_f(b, &sp[j].f), " n=");
        wnum(b, mol[j], " mol");
    }
    /* extent: smallest moles/coefficient among the given reactants */
    for (j = 0; j < nsp; j++) {
        num_t x;
        if (!sp[j].given || (ng_re && j >= nre)) continue;
        x = num_div(mol[j], num_exact(sp[j].coef), false);
        if (lim == 0xFF || x.v < xi.v * (1 - 1e-9)) { tie = false; xi = x; lim = j; }
        else if (x.v <= xi.v * (1 + 1e-9)) tie = true;
    }
    if (lim == 0xFF) return false;
    wnum("n/coef min=", xi, NULL);
    if (ng_re > 1) {
        if (tie) strcpy(b, "none left");
        else put(put_f(b, &sp[lim].f), " limiting");
        row(b);
        wline(b);
        for (j = 0; j < nre; j++) {
            num_t used, have;
            if (!sp[j].given || j == lim) continue;
            u = show_unit(&sp[j], kind[j]);
            from_mol(num_mul(xi, num_exact(sp[j].coef), false), &sp[j].f, u, &used);
            from_mol(mol[j], &sp[j].f, u, &have);
            amount_text(num_sub(have, used, false), u, &sp[j].f, " left", b, alt);
            if (!tie) row(b);
            wline(b);
        }
    }
    if (want) {                            /* "mol o2", "g n2", "100.0g c9h8o" */
        const char *s = want, *end = want + wlen, *last;
        qty_t act;
        bool has_act = false;
        unit_t wu;
        int8_t w;
        num_t n;
        while (s < end && *s == ' ') s++;
        while (end > s && end[-1] == ' ') end--;
        for (last = end; last > s && last[-1] != ' '; last--) ;
        if ((w = find(last, (uint8_t)(end - last))) < 0) return false;
        memset(&wu, 0, sizeof wu);
        if (last > s) {
            uint8_t tl = (uint8_t)(last - 1 - s);
            num_t t;
            if (num_parse(s, &t)) {
                if (qty_parse(s, &act) != tl) return false;
                has_act = true;
                wu = act.u;
                unit_resolve_m(&wu, true);
            } else if (unit_parse(s, &wu) != tl) return false;
        }
        if (unit_is_empty(&wu) && !has_act) wu = kind[lim] == 1 ? u_g : u_mol;
        if (!from_mol(num_mul(xi, num_exact(sp[w].coef), false), &sp[w].f, &wu, &n)) return false;
        amount_text(n, &wu, &sp[w].f, has_act ? " theor." : 0, b, small);
        wline(b);
        if (has_act) {                     /* actual yield -> percent yield */
            n = num_mul(num_div(act.n, n, false), num_exact(100), false);
            num_fmt_answer(num_round(n), b, small);
            strcat(b, " %");
            memset(&wu, 0, sizeof wu);
        }
        row(b);
        mem.ans.n = num_round(n);
        mem.ans.u = wu;
        mem.has_ans = true;
        return true;
    }
    /* theoretical yield of each product (first two on screen) */
    u = kind[lim] == 1 ? &u_g : &u_mol;
    for (j = nre; j < nsp; j++) {
        num_t n;
        if (!from_mol(num_mul(xi, num_exact(sp[j].coef), false), &sp[j].f, u, &n)) return false;
        amount_text(n, u, &sp[j].f, 0, b, alt);
        if (j == nre) {
            strcpy(small, alt);
            mem.ans.n = num_round(n);
            mem.ans.u = *u;
            mem.has_ans = true;
        }
        if (j < nre + 2) row(b);
        wline(b);
    }
    return true;
}

/* ---------- the app ---------- */

/* home-screen math: "Ans*.835", "*.835", "2.5*3" */
NOINLINE static bool calc(const char *in, char *out, char *small) {
    expr_ctx_t c;
    qty_t q;
    char m[32], us[16];
    memset(&c, 0, sizeof c);
    c.ans = mem.ans;
    c.has_ans = mem.has_ans;
    c.work = wk;
    if (!expr_eval(in, &c, &q)) return false;
    mem.ans = q;
    mem.has_ans = true;
    num_fmt_answer(q.n, m, small);
    put(out, m);
    if (unit_str(&q.u, us)) put(put(out + strlen(out), " "), us);
    return true;
}

NOINLINE static void react(const char *in, char *out, char *small) {
    const char *sec[3], *p;
    uint8_t len[3], ns = 0, i;
    bool any = false, want = false;
    sec[0] = in;
    for (p = in; *p; p++)
        if (*p == CH_STO || *p == '=') {
            if (ns == 2) goto bad;
            len[ns] = (uint8_t)(p - sec[ns]);
            sec[++ns] = p + 1;
        }
    len[ns] = (uint8_t)(p - sec[ns]);
    ns++;
    if (!side(sec[0], len[0]) || !(nre = nsp)) goto bad;
    want = ns == 3;
    if (ns > 1 && !side(sec[1], len[1])) {
        if (ns == 3) goto bad;
        /* "25.0ml .100m hcl+naoh→g h2o": the products are predicted */
        nsp = nre;
        sec[2] = sec[1];
        len[2] = len[1];
        ns = 1;
        want = true;
    }
    for (i = 0; i < nsp; i++) any |= sp[i].given;

    if (ns == 1 && !predict()) {          /* no products typed: predict them */
        row(why ? "no reaction" : "type " "\x1C" "products");
        if (why) row(why);
        return;
    }
    if (nsp == nre) goto bad;
    if (!balance()) { strcpy(out, "ERR:CAN'T BALANCE"); return; }
    {
        char b[100];
        equation(b);
        wline(b);
    }
    if (any) {
        if (!stoich(want ? sec[2] : 0, want ? len[2] : 0, small)) {
            op = obeg; *out = 0;
            strcpy(out, "ERR:UNITS");
        }
        atom_work();
        return;
    }
    if (want) { strcpy(out, "ERR:NO AMOUNT"); return; }
    eq_rows();
    {
        const char *t = rtype();
        char *q;
        int32_t sum = 0;
        for (i = 0; i < nsp; i++) sum += sp[i].coef;
        q = put_int(put(tmp, "sum "), sum);
        if (t && strlen(tmp) + 1 + strlen(t) <= UI_COLS) {
            put(put(q, " "), t);
            row(tmp);
        } else {
            row(tmp);
            if (t) row(t);
        }
        if (t && strcmp(t, "combination") && strcmp(t, "decomposition") && strcmp(t, "combustion"))
            ion_rows(t[0] != 's');
        atom_work();
    }
    return;
bad:
    out[0] = 0;
    if (!calc(in, out, small)) strcpy(out, "ERR:SYNTAX");
}

static void compute(const char *in, char *out, char *small, work_t *w) {
    obeg = op = out;
    oend = out + UI_OUT - 1;
    *out = 0;
    wk = w;
    nsp = nre = 0;
    unit_parse("mol", &u_mol);
    unit_parse("g", &u_g);
    if (!strncmp(in, "Ans", 3) || expr_starts_with_op(in)) {
        if (!calc(in, out, small)) strcpy(out, "ERR:SYNTAX");
        return;
    }
    react(in, out, small);
}

int main(void) {
    static const app_def_t d = {"REACT", "REACTst", SMART_ALL, compute, 0, &mem, sizeof mem};
    app_run(&d);
    return 0;
}
