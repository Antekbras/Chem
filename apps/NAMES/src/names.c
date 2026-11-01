/* names.c - NAMES engine (replaces NOMENC "Formula -> name" and
 * "Name -> formula"; its Tables and Atom charge screens were dropped).
 *
 * Covers ionic compounds (Roman numerals for metals with more than one
 * charge), binary covalent compounds (mono..deca prefixes), acids
 * (hydro-ic, -ic, -ous), ions, hydrates, elements and common names.
 * The answer's second row is an info line: ionic/covalent, solubility,
 * ions in water, acid/base strength, electrolyte, VSEPR shape. [GRAPH]
 * shows how the name or formula was worked out. */
#include <string.h>
#include "names.h"
#include "ui.h"
#include "formula.h"
#include "num.h"
#include "ionic.h"

/* ---------------------------------------------------------------- data */

/* ions the shared table lacks (NOMENC had 48) */
static const polyion_t xions[] = {
    {"OCN", -1, "cyanate"}, {"N3", -1, "azide"}, {"BrO2", -1, "bromite"},
    {"BrO", -1, "hypobromite"}, {"IO2", -1, "iodite"}, {"IO", -1, "hypoiodite"},
    {"HS", -1, "bisulfide"}, {"HSO3", -1, "hydrogen sulfite"},
    {"SeO3", -2, "selenite"}, {"S2O8", -2, "persulfate"}, {"SiO3", -2, "silicate"},
    {"AsO4", -3, "arsenate"}, {"AsO3", -3, "arsenite"}, {"BO3", -3, "borate"},
    {"C6H5O7", -3, "citrate"}, {"CH3COO", -1, "acetate"},
};
#define NX ((uint8_t)(sizeof xions / sizeof xions[0]))
#define NION ((uint8_t)(NUM_POLYIONS + NX))

static const polyion_t *ion(uint8_t i) {
    return i < NUM_POLYIONS ? &polyions[i] : &xions[i - NUM_POLYIONS];
}

static const char *const alias[][2] = {
    {"bicarbonate", "hydrogen carbonate"}, {"bisulfate", "hydrogen sulfate"},
    {"bisulfite", "hydrogen sulfite"},
};

/* monatomic anions: Z, -ide root, charge */
static const struct { uint8_t z; char root[8]; int8_t q; } roots[] = {
    {9, "fluor", -1}, {17, "chlor", -1}, {35, "brom", -1}, {53, "iod", -1},
    {8, "ox", -2}, {16, "sulf", -2}, {34, "selen", -2}, {52, "tellur", -2},
    {7, "nitr", -3}, {15, "phosph", -3}, {33, "arsen", -3}, {5, "bor", -3},
    {6, "carb", -4}, {14, "silic", -4}, {1, "hydr", -1}, {85, "astat", -1},
    {51, "antimon", -3},
};
#define NROOT ((uint8_t)(sizeof roots / sizeof roots[0]))

/* common names. cls: c covalent, b weak base, a weak acid, i ionic.
 * Upper case: the systematic name comes first and this one is "also". */
static const struct { const char *f, *name; char cls; } common[] = {
    {"H2O", "water", 'c'}, {"NH3", "ammonia", 'b'}, {"CH4", "methane", 'c'},
    {"H2O2", "hydrogen peroxide", 'c'}, {"O3", "ozone", 'c'}, {"PH3", "phosphine", 'c'},
    {"C6H12O6", "glucose", 'c'}, {"C12H22O11", "sucrose", 'c'},
    {"CH3COOH", "acetic acid", 'a'}, {"C2H5OH", "ethanol", 'c'}, {"CH3OH", "methanol", 'c'},
    {"C2H6", "ethane", 'c'}, {"C3H8", "propane", 'c'}, {"C4H10", "butane", 'c'},
    {"C5H12", "pentane", 'c'}, {"C6H14", "hexane", 'c'}, {"C8H18", "octane", 'c'},
    {"C2H2", "acetylene", 'c'}, {"C2H4", "ethylene", 'c'}, {"N2H4", "hydrazine", 'b'},
    {"CaC2", "calcium carbide", 'i'}, {"N2O", "nitrous oxide", 'C'}, {"NO", "nitric oxide", 'C'},
};
#define NCOMMON ((uint8_t)(sizeof common / sizeof common[0]))

static const char *const prefix[] = {
    "mono", "di", "tri", "tetra", "penta", "hexa", "hepta", "octa", "nona", "deca"};
static const char *const roman[] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII"};

/* ------------------------------------------------------------- helpers */

typedef struct { uint8_t z; int8_t pi; int8_t q; } part_t;   /* element z or ion pi */

enum { NK_NONE, NK_IONIC, NK_ACID, NK_COV, NK_ELEM, NK_ION, NK_COMMON };

typedef struct {
    uint8_t kind, nc, na, hyd, cm;    /* cm: common-name index + 1 */
    bool from_name, guessed;          /* name -> formula; metal charge assumed */
    part_t c, a;
} cmpd_t;

static char *cpy(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }

static char *put_uint(char *p, unsigned v) {
    if (v >= 10) p = put_uint(p, v / 10);
    *p++ = (char)('0' + v % 10);
    *p = 0;
    return p;
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_low(char c) { return c >= 'a' && c <= 'z'; }

static bool variable(uint8_t z) {
    uint8_t i, n = 0;
    for (i = 0; i < 3; i++) if (elements[z].ions[i] > 0) n++;
    return n > 1;
}

static int8_t first_pos(uint8_t z) {
    uint8_t i;
    for (i = 0; i < 3; i++) if (elements[z].ions[i] > 0) return elements[z].ions[i];
    return 0;
}

static int8_t root_of(uint8_t z) {
    uint8_t i;
    for (i = 0; i < NROOT; i++) if (roots[i].z == z) return (int8_t)i;
    return -1;
}

static int8_t ion_find(const char *t, uint8_t l, int8_t sign) {
    uint8_t i;
    for (i = 0; i < NION; i++) {
        const polyion_t *p = ion(i);
        if ((sign > 0) != (p->charge > 0) && sign) continue;
        if (strlen(p->formula) == l && !memcmp(p->formula, t, l)) return (int8_t)i;
    }
    return -1;
}

static const char *part_text(const part_t *x) {
    return x->pi >= 0 ? ion((uint8_t)x->pi)->formula : elements[x->z].sym;
}

static char *put_charge(char *p, int8_t q) {
    int8_t a = q < 0 ? (int8_t)-q : q;
    if (a > 1) {
        if (is_digit(p[-1])) *p++ = ' ';
        *p++ = (char)('0' + a);
    }
    *p++ = q > 0 ? '+' : '-';
    *p = 0;
    return p;
}

static char *put_part(char *p, const part_t *x, uint8_t n) {
    bool par = x->pi >= 0 && n > 1;
    if (par) *p++ = '(';
    p = cpy(p, part_text(x));
    if (par) *p++ = ')';
    if (n > 1) p = put_uint(p, n);
    *p = 0;
    return p;
}

/* "Fe3+", "SO4 2-" */
static char *put_ion(char *p, const part_t *x) {
    p = cpy(p, part_text(x));
    return put_charge(p, x->q);
}

/* "ox" + "ide", or the ion's own name */
static char *put_anion_name(char *p, const part_t *a) {
    if (a->pi >= 0) return cpy(p, ion((uint8_t)a->pi)->name);
    p = cpy(p, roots[root_of(a->z)].root);
    return cpy(p, "ide");
}

static char *put_cation_name(char *p, const part_t *c) {
    if (c->pi >= 0) return cpy(p, ion((uint8_t)c->pi)->name);
    p = cpy(p, elements[c->z].name);
    if (elem_is_metal(c->z) && variable(c->z) && c->q >= 1 && c->q <= 8) {
        *p++ = '(';
        p = cpy(p, roman[c->q - 1]);
        *p++ = ')';
        *p = 0;
    }
    return p;
}

/* -ide -> hydro-ic, -ate -> -ic, -ite -> -ous ; false if no acid name */
static bool acid_name(const part_t *a, char *p) {
    char stem[24], *s;
    const char *end;
    if (a->pi < 0) {
        if (a->z == 1 || a->z == 8) return false;
        s = cpy(stem, roots[root_of(a->z)].root);
        end = "ic";
        p = cpy(p, "hydro");
    } else {
        const char *n = ion((uint8_t)a->pi)->name;
        uint8_t l = (uint8_t)strlen(n);
        if (a->q > 0 || strchr(n, ' ') || l < 4 || l > 20) return false;
        if (!strcmp(n, "hydroxide") || !strcmp(n, "peroxide")) return false;
        memcpy(stem, n, l - 3);
        s = stem + l - 3;
        *s = 0;
        if (!strcmp(n + l - 3, "ide")) {
            p = cpy(p, !strcmp(n, "azide") ? "hydraz" : "hydro");
            if (!strcmp(n, "azide")) { stem[0] = 'o'; s = stem + 1; }
            end = "ic";
        } else if (!strcmp(n + l - 3, "ate")) end = "ic";
        else if (!strcmp(n + l - 3, "ite")) end = "ous";
        else return false;
    }
    *s = 0;
    if (s - stem >= 4 && !strcmp(s - 4, "sulf")) s = cpy(s, "ur");
    else if (s - stem >= 6 && !strcmp(s - 6, "phosph")) s = cpy(s, "or");
    p = cpy(p, stem);
    p = cpy(p, end);
    cpy(p, " acid");
    return true;
}

/* prefix k (1..10) on word w: chars to skip, 0 if no match. alt: elided
 * form (tetr+oxide) */
static uint8_t pre_len(const char *w, uint8_t l, uint8_t k, bool alt) {
    const char *p = prefix[k - 1];
    uint8_t pl = (uint8_t)strlen(p);
    if (!alt) return (l > pl && !memcmp(w, p, pl)) ? pl : 0;
    if ((p[pl - 1] == 'a' || p[pl - 1] == 'o') && l > pl && !memcmp(w, p, pl - 1) && w[pl - 1] == 'o')
        return (uint8_t)(pl - 1);
    return 0;
}

static char *put_prefix(char *p, uint8_t n, bool first, const char *next) {
    const char *x;
    uint8_t l;
    if (n < 1 || n > 10 || (first && n == 1)) return p;
    x = prefix[n - 1];
    l = (uint8_t)strlen(x);
    memcpy(p, x, l);
    /* a or o before oxide drops: monoxide, tetroxide */
    if (next[0] == 'o' && (x[l - 1] == 'a' || x[l - 1] == 'o')) l--;
    p += l;
    *p = 0;
    return p;
}

/* ------------------------------------------------- names and formulas */

static void cmpd_name(const cmpd_t *c, char *p) {
    switch (c->kind) {
    case NK_COMMON:
        p = cpy(p, common[c->cm - 1].name);
        break;
    case NK_IONIC:
        p = put_cation_name(p, &c->c);
        *p++ = ' ';
        p = put_anion_name(p, &c->a);
        break;
    case NK_ACID:
        acid_name(&c->a, p);
        p += strlen(p);
        break;
    case NK_COV: {
        const char *r = roots[root_of(c->a.z)].root;
        p = put_prefix(p, c->nc, true, elements[c->c.z].name);
        p = cpy(p, elements[c->c.z].name);
        *p++ = ' ';
        p = put_prefix(p, c->na, false, r);
        p = cpy(p, r);
        p = cpy(p, "ide");
        break;
    }
    case NK_ELEM:
        p = cpy(p, elements[c->c.z].name);
        break;
    case NK_ION:
        if (c->c.q > 0) p = put_cation_name(p, &c->c);
        else p = put_anion_name(p, &c->c);
        p = cpy(p, " ion");
        break;
    }
    if (c->hyd) {
        *p++ = ' ';
        p = put_prefix(p, c->hyd, false, "h");
        cpy(p, "hydrate");
    }
}

static bool diatomic(uint8_t z) {
    return z == 1 || z == 7 || z == 8 || z == 9 || z == 17 || z == 35 || z == 53;
}

/* ASCII formula, canonical case: "Fe2(SO4)3", "Fe3+", "CuSO4·5H2O" */
static void cmpd_formula(const cmpd_t *c, char *p) {
    switch (c->kind) {
    case NK_COMMON:
        p = cpy(p, common[c->cm - 1].f);
        break;
    case NK_ELEM:
        p = cpy(p, elements[c->c.z].sym);
        if (diatomic(c->c.z)) p = cpy(p, "2");
        break;
    case NK_ION:
        p = put_ion(p, &c->c);
        break;
    default:
        p = put_part(p, &c->c, c->nc);
        p = put_part(p, &c->a, c->na);
    }
    if (c->hyd) {
        *p++ = CH_DOT;
        if (c->hyd > 1) p = put_uint(p, c->hyd);
        cpy(p, "H2O");
    }
}

/* element counts of a canonical-case formula ("Fe2(SO4)3") */
static void count(const char *s, const char *end, uint16_t m, formula_t *f) {
    while (s < end && *s && *s != (char)CH_DOT) {
        uint16_t k = 0;
        if (*s == '(') {
            const char *q = s + 1, *close;
            int depth = 1;
            while (q < end && depth) {
                if (*q == '(') depth++;
                else if (*q == ')') depth--;
                q++;
            }
            close = q - 1;
            while (is_digit(*q)) k = (uint16_t)(k * 10 + *q++ - '0');
            count(s + 1, close, (uint16_t)(m * (k ? k : 1)), f);
            s = q;
        } else {
            uint8_t l = (uint8_t)(is_low(s[1]) ? 2 : 1), z = elem_find(s, l), i;
            s += l;
            while (is_digit(*s)) k = (uint16_t)(k * 10 + *s++ - '0');
            if (!z) return;
            if (!k) k = 1;
            for (i = 0; i < f->nel && f->el[i].z != z; i++) ;
            if (i == f->nel && f->nel < F_MAXEL) { f->el[i].z = z; f->el[i].n = 0; f->nel++; }
            if (i < F_MAXEL) f->el[i].n = (uint16_t)(f->el[i].n + k * m);
        }
    }
}

/* ------------------------------------------------------ formula -> name */

/* one top-level unit: "(G)n" or "Sym n" */
static uint8_t unit(const char *s, uint8_t l, const char **t, uint8_t *tl, uint8_t *n, bool *grp) {
    uint8_t i = 0, k = 0;
    *grp = false;
    if (!l) return 0;
    if (s[0] == '(') {
        uint8_t d = 0;
        for (i = 0; i < l; i++) {
            if (s[i] == '(') d++;
            else if (s[i] == ')' && !--d) break;
        }
        if (i == l) return 0;
        *t = s + 1;
        *tl = (uint8_t)(i - 1);
        *grp = true;
        i++;
    } else {
        *t = s;
        *tl = (uint8_t)(l > 1 && is_low(s[1]) ? 2 : 1);
        i = *tl;
    }
    while (i < l && is_digit(s[i])) k = (uint8_t)(k * 10 + s[i++] - '0');
    *n = k ? k : 1;
    return i;
}

/* split a neutral canonical formula into cation + anion */
static bool decompose(const char *d, uint8_t len, uint8_t nel, cmpd_t *c) {
    const char *t;
    uint8_t tl, n, used, rl, k;
    bool grp;
    part_t cand[2];
    uint8_t cn[2], nc = 0;
    const char *r;

    memset(c, 0, sizeof *c);
    c->c.pi = c->a.pi = -1;
    /* cation: (NH4)2, NH4, Hg2.. or the first element */
    used = unit(d, len, &t, &tl, &n, &grp);
    if (!used) return false;
    if (grp) {
        c->c.pi = ion_find(t, tl, 1);
        if (c->c.pi < 0) return false;
        c->nc = n;
    } else if (len > 3 && (c->c.pi = ion_find(d, 3, 1)) >= 0 && !is_low(d[3]) && !is_digit(d[3]) &&
               ion((uint8_t)c->c.pi)->formula[2] != '2') {
        used = 3;                               /* NH4Cl, H3O+ */
        c->nc = 1;
    } else {
        c->c.pi = -1;
        c->c.z = elem_find(t, tl);
        c->nc = n;
    }
    r = d + used;
    rl = (uint8_t)(len - used);
    if (!rl) return false;

    /* anion candidates: a lone element (oxide), then a polyatomic ion (peroxide) */
    k = unit(r, rl, &t, &tl, &n, &grp);
    if (k == rl) {
        int8_t ri;
        if (grp) {
            int8_t pi = ion_find(t, tl, -1);
            if (pi >= 0) { cand[nc].pi = pi; cand[nc].z = 0; cand[nc].q = ion((uint8_t)pi)->charge; cn[nc++] = n; }
        } else if ((ri = root_of(elem_find(t, tl))) >= 0) {
            cand[nc].pi = -1; cand[nc].z = roots[ri].z; cand[nc].q = roots[ri].q; cn[nc++] = n;
        }
    }
    {
        int8_t pi = ion_find(r, rl, -1);
        if (pi >= 0) { cand[nc].pi = pi; cand[nc].z = 0; cand[nc].q = ion((uint8_t)pi)->charge; cn[nc++] = 1; }
    }
    if (!nc) return false;

    if (c->c.pi >= 0 || elem_is_metal(c->c.z)) {
        /* ionic: the cation's charge balances the anions */
        uint8_t i, pick = 0xFF;
        for (i = 0; i < nc; i++) {
            unsigned tot = (unsigned)(-cand[i].q) * cn[i];
            int8_t q;
            bool good;
            if (tot % c->nc) continue;
            q = (int8_t)(tot / c->nc);
            if (q < 1 || q > 8) continue;
            if (c->c.pi >= 0) good = q == ion((uint8_t)c->c.pi)->charge;
            else good = variable(c->c.z) || q == first_pos(c->c.z);
            if (good) { pick = i; break; }
            if (pick == 0xFF) pick = (uint8_t)(i | 0x80);
        }
        if (pick == 0xFF) return false;
        pick &= 0x7F;
        c->a = cand[pick];
        c->na = cn[pick];
        c->c.q = (int8_t)((unsigned)(-c->a.q) * c->na / c->nc);
        c->kind = NK_IONIC;
        return true;
    }
    if (c->c.z == 1) {                          /* acid: H+ balances the anion */
        uint8_t i;
        for (i = 0; i < nc; i++) {
            char tmp[32];
            if ((unsigned)(-cand[i].q) * cn[i] != c->nc) continue;
            if (!acid_name(&cand[i], tmp)) continue;
            c->a = cand[i];
            c->na = cn[i];
            c->c.q = 1;
            c->kind = NK_ACID;
            return true;
        }
    }
    /* binary covalent: two nonmetals */
    if (nel == 2 && cand[0].pi < 0 && c->c.z != cand[0].z && root_of(cand[0].z) >= 0 &&
        !elem_is_metal(c->c.z)) {
        c->a = cand[0];
        c->na = cn[0];
        c->kind = NK_COV;
        return true;
    }
    return false;
}

/* ------------------------------------------------------ name -> formula */

static bool word_eq(const char *s, uint8_t l, const char *w) {
    return strlen(w) == l && !memcmp(s, w, l);
}

/* anion name: "oxide", "nitrate", "hydrogen carbonate", "bicarbonate" */
static bool anion_name(const char *s, uint8_t l, part_t *a) {
    uint8_t i;
    for (i = 0; i < sizeof alias / sizeof alias[0]; i++)
        if (word_eq(s, l, alias[i][0])) { s = alias[i][1]; l = (uint8_t)strlen(s); }
    for (i = 0; i < NION; i++) {
        if (ion(i)->charge < 0 && word_eq(s, l, ion(i)->name)) {
            a->pi = (int8_t)i; a->z = 0; a->q = ion(i)->charge;
            return true;
        }
    }
    for (i = 0; i < NROOT; i++) {
        uint8_t rl = (uint8_t)strlen(roots[i].root);
        if (l == rl + 3 && !memcmp(s, roots[i].root, rl) && !memcmp(s + rl, "ide", 3)) {
            a->pi = -1; a->z = roots[i].z; a->q = roots[i].q;
            return true;
        }
    }
    return false;
}

/* charge written after a cation: "3", "(3)", "(iii)", "iii" ; 0 if none */
static int8_t charge_text(const char *s, uint8_t l) {
    static const char *const rl[] = {"i", "ii", "iii", "iv", "v", "vi", "vii", "viii"};
    uint8_t i;
    if (l >= 2 && s[0] == '(' && s[l - 1] == ')') { s++; l = (uint8_t)(l - 2); }
    if (l == 1 && s[0] >= '1' && s[0] <= '8') return (int8_t)(s[0] - '0');
    for (i = 0; i < 8; i++) if (word_eq(s, l, rl[i])) return (int8_t)(i + 1);
    return 0;
}

typedef bool (*resolve_t)(const char *s, uint8_t l, part_t *x);

static bool cation_word(const char *s, uint8_t l, part_t *x) {
    x->pi = -1;
    x->q = 0;
    if (word_eq(s, l, "ammonium")) { x->pi = ion_find("NH4", 3, 1); x->q = 1; return true; }
    if (word_eq(s, l, "hydronium")) { x->pi = ion_find("H3O", 3, 1); x->q = 1; return true; }
    x->z = elem_find_name(s, l);
    return x->z != 0;
}

/* word with an optional covalent prefix: 1 = no prefix, 0x80|n = prefix n */
static uint8_t with_prefix(const char *s, uint8_t l, resolve_t ok, part_t *x) {
    uint8_t k;
    if (ok(s, l, x)) return 1;
    for (k = 1; k <= 10; k++) {
        uint8_t alt;
        for (alt = 0; alt < 2; alt++) {
            uint8_t p = pre_len(s, l, k, alt);
            if (p && ok(s + p, (uint8_t)(l - p), x)) return (uint8_t)(k | 0x80);
        }
    }
    return 0;
}

static uint8_t lcm_div(uint8_t a, uint8_t b) {      /* lcm(a,b)/a */
    uint8_t x = a, y = b;
    while (y) { uint8_t t = (uint8_t)(x % y); x = y; y = t; }
    return (uint8_t)(b / x);
}

static bool name_parse(char *s, cmpd_t *c) {
    uint8_t l = (uint8_t)strlen(s), i, wl;
    bool ion_word = false;
    char *w;
    part_t x;
    memset(c, 0, sizeof *c);
    c->c.pi = c->a.pi = -1;
    while (l && s[l - 1] == ' ') s[--l] = 0;
    while (*s == ' ') { s++; l--; }
    if (!l) return false;

    for (i = 0; i < NCOMMON; i++)
        if (word_eq(s, l, common[i].name)) { c->kind = NK_COMMON; c->cm = (uint8_t)(i + 1); return true; }

    /* "hydrogen ion", "iron(iii) ion" */
    if (l > 4 && !strcmp(s + l - 4, " ion")) {
        l = (uint8_t)(l - 4);
        s[l] = 0;
        ion_word = true;
    }

    /* hydrate: last word "<prefix>hydrate" */
    w = strrchr(s, ' ');
    if (ion_word) w = NULL;
    if (w && l - (w + 1 - s) >= 7 && !strcmp(s + l - 7, "hydrate")) {
        uint8_t pl = (uint8_t)(s + l - 7 - (w + 1)), k;
        for (k = 1; k <= 10; k++)
            if (strlen(prefix[k - 1]) == pl && !memcmp(w + 1, prefix[k - 1], pl)) break;
        if (k > 10 && pl) return false;
        c->hyd = k > 10 ? 1 : k;
        *w = 0;
        l = (uint8_t)(w - s);
        while (l && s[l - 1] == ' ') s[--l] = 0;
    }

    /* acids */
    if (l > 5 && !strcmp(s + l - 5, " acid")) {
        char tmp[32];
        for (i = 0; i < NION + NROOT; i++) {
            part_t a;
            if (i < NION) {
                if (ion(i)->charge > 0) continue;
                a.pi = (int8_t)i; a.z = 0; a.q = ion(i)->charge;
            } else {
                a.pi = -1; a.z = roots[i - NION].z; a.q = roots[i - NION].q;
            }
            if (acid_name(&a, tmp) && !strcmp(tmp, s)) {
                c->kind = NK_ACID;
                c->c.z = 1; c->c.q = 1;
                c->a = a;
                c->nc = (uint8_t)-a.q;
                c->na = 1;
                return true;
            }
        }
        return false;
    }

    /* a lone anion: "nitrate", "chloride", "hydrogen carbonate" */
    if (anion_name(s, l, &x)) { c->kind = NK_ION; c->c = x; return true; }

    /* cation word (with optional prefix and charge) */
    {
        uint8_t ll = 0, p1, rest;
        int8_t q = 0;
        while (ll < l && is_low(s[ll])) ll++;
        p1 = with_prefix(s, ll, cation_word, &c->c);
        if (!p1) return false;
        rest = ll;
        if (rest < l && s[rest] != ' ') {            /* iron3, iron(iii) */
            uint8_t e = rest;
            while (e < l && s[e] != ' ') e++;
            q = charge_text(s + rest, (uint8_t)(e - rest));
            if (!q) return false;
            rest = e;
        }
        while (rest < l && s[rest] == ' ') rest++;
        if (!q && rest < l) {                        /* iron (iii) oxide, iron iii oxide */
            uint8_t e = rest;
            while (e < l && s[e] != ' ') e++;
            if ((q = charge_text(s + rest, (uint8_t)(e - rest))) != 0) {
                rest = e;
                while (rest < l && s[rest] == ' ') rest++;
            }
        }
        if (q && c->c.pi < 0) c->c.q = q;
        if (c->c.z == 80 && q == 1) {               /* mercury(I) is Hg2 2+ */
            c->c.pi = ion_find("Hg2", 3, 1);
            c->c.q = 2;
        }
        if (rest >= l) {                             /* "iron", "iron3", "ammonium" */
            if (c->hyd || p1 & 0x80) return false;
            if (c->c.pi >= 0 || q || ion_word) {
                if (!c->c.q && !first_pos(c->c.z)) c->c.q = elements[c->c.z].ions[0];
                if (!c->c.q) c->c.q = first_pos(c->c.z);
                c->kind = NK_ION;
            } else c->kind = NK_ELEM;
            return true;
        }
        /* anion */
        wl = (uint8_t)(l - rest);
        {
            uint8_t p2 = with_prefix(s + rest, wl, anion_name, &c->a);
            bool metal = c->c.pi >= 0 || elem_is_metal(c->c.z);
            if (!p2) return false;
            if (metal || (c->c.z == 1 && !(p1 & 0x80) && !(p2 & 0x80))) {
                /* ionic, or "hydrogen chloride": charges cross */
                uint8_t qa = (uint8_t)-c->a.q, qc;
                if (!c->c.q) {
                    c->c.q = c->c.z == 1 ? 1 : first_pos(c->c.z);
                    c->guessed = c->c.pi < 0 && variable(c->c.z);
                }
                qc = (uint8_t)c->c.q;
                if (!qc) return false;
                c->nc = lcm_div(qc, qa);
                c->na = lcm_div(qa, qc);
                c->kind = c->c.z == 1 ? NK_ACID : NK_IONIC;
                if (c->kind == NK_ACID) {
                    char tmp[32];
                    if (!acid_name(&c->a, tmp)) c->kind = NK_COV;
                }
                return true;
            }
            if (c->a.pi >= 0) return false;
            c->nc = (uint8_t)(p1 & 0x7F);
            c->na = (uint8_t)(p2 & 0x7F);
            if (!(p1 & 0x80)) c->nc = 1;
            if (!(p2 & 0x80)) c->na = 1;
            c->kind = NK_COV;
            return true;
        }
    }
}

/* --------------------------------------------------------------- info */

static uint8_t valence(uint8_t z) {
    econfig_t e;
    uint8_t i, n = 0, v = 0;
    if (!econfig(z, 0, &e)) return 0;
    for (i = 0; i < NUM_SUBSHELLS; i++) if (e.occ[i] && sub_n[i] > n) n = sub_n[i];
    for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == n && sub_l[i] <= 1) v += e.occ[i];
    return v;
}

typedef struct { const char *name, *angle; bool polar; uint8_t groups, lp; } shape_t;

/* VSEPR for one central atom with terminal atoms (CCl4, NH3, SO4 2-) */
static bool vsepr(const formula_t *f, int8_t q, shape_t *sh) {
    uint8_t i, ci = 0xFF, atoms = 0;
    int v = -q, used = 0;
    for (i = 0; i < f->nel; i++) {
        uint8_t z = f->el[i].z;
        atoms = (uint8_t)(atoms + f->el[i].n);
        if (f->el[i].n == 1 && z != 1 &&
            (ci == 0xFF || elements[z].en100 < elements[f->el[ci].z].en100)) ci = i;
    }
    if (atoms == 2) {
        sh->name = "linear"; sh->angle = "180\x14";
        sh->polar = f->nel == 2; sh->groups = 0; sh->lp = 0;
        return true;
    }
    if (ci == 0xFF || atoms > 7) return false;
    for (i = 0; i < f->nel; i++) {
        v += valence(f->el[i].z) * f->el[i].n;
        if (i != ci) used += (f->el[i].z == 1 ? 2 : 8) * f->el[i].n;
    }
    if (v < used || (v - used) % 2) return false;
    sh->lp = (uint8_t)((v - used) / 2);
    sh->groups = (uint8_t)(atoms - 1 + sh->lp);
    {
        static const struct { uint8_t g, lp; const char *name, *angle; } t[] = {
            {2, 0, "linear", "180\x14"}, {3, 0, "trig. planar", "120\x14"},
            {3, 1, "bent", "<120\x14"}, {4, 0, "tetrahedral", "109.5\x14"},
            {4, 1, "trig. pyramidal", "<109.5\x14"}, {4, 2, "bent", "<109.5\x14"},
            {5, 0, "trig. bipyramidal", "90\x14 120\x14"}, {5, 1, "seesaw", "<90\x14 <120\x14"},
            {5, 2, "T-shaped", "<90\x14"}, {5, 3, "linear", "180\x14"},
            {6, 0, "octahedral", "90\x14"}, {6, 1, "sq. pyramidal", "<90\x14"},
            {6, 2, "sq. planar", "90\x14"},
        };
        for (i = 0; i < sizeof t / sizeof t[0]; i++) {
            if (t[i].g == sh->groups && t[i].lp == sh->lp) {
                bool sym = t[i].lp == 0 || (t[i].g == 5 && t[i].lp == 3) || (t[i].g == 6 && t[i].lp == 2);
                sh->name = t[i].name;
                sh->angle = t[i].angle;
                sh->polar = !(sym && f->nel == 2);
                return true;
            }
        }
    }
    return false;
}

/* the shared solubility / electrolyte rules (ionic.c), fed with this
 * app's split, which knows more ions (citrate, azide, peroxide) */
static uint8_t el_class(const cmpd_t *c, const char *fa) {
    formula_t f;
    ionic_t o;
    memset(&o, 0, sizeof o);
    memset(&f, 0, sizeof f);
    strcpy(f.disp, fa);
    o.kind = c->kind == NK_ACID ? IK_ACID : IK_IONIC;
    strcpy(o.cat, part_text(&c->c));
    strcpy(o.an, part_text(&c->a));
    o.zc = c->c.pi < 0 ? c->c.z : 0;
    o.za = c->a.pi < 0 ? c->a.z : 0;
    o.qc = c->c.q; o.qa = c->a.q;
    o.nc = c->nc; o.na = c->na;
    return ionic_class(&f, &o);
}

/* append ", text" to the info row if it still fits */
static void add_tag(char *row, const char *sep, const char *t) {
    if (strlen(row) + strlen(sep) + strlen(t) <= UI_COLS) { strcat(row, sep); strcat(row, t); }
}

static void put_mass(const formula_t *f, work_t *w) {
    char b[24];
    fp_t m = formula_mass(f);
    if (m <= 0) return;
    num_fmt_plain(num_with_sf(m, m >= 100 ? 5 : 4), b);
    work_add(w, "M = ", b, " g/mol", NULL);
}

static void info(const cmpd_t *c, const char *fa, const formula_t *f, char *row, work_t *w) {
    char a[32], b[32];
    shape_t sh;
    bool shp = c->kind != NK_IONIC && vsepr(f, c->kind == NK_ION ? c->c.q : 0, &sh);
    row[0] = 0;
    switch (c->kind) {
    case NK_IONIC: {
        uint8_t s = el_class(c, fa);
        bool base = s == EL_STRONG_BASE;
        put_ion(a, &c->c);
        put_ion(b, &c->a);
        formula_pretty(a, a);
        formula_pretty(b, b);
        work_add(w, "ionic: ", c->c.pi >= 0 || c->a.pi >= 0 ? "has polyatomic ion" : "metal + nonmetal",
                 NULL, NULL);
        if (c->from_name) {
            /* charges cross: Fe3+ O2- -> Fe2O3 */
            char t[32], *p;
            if (c->guessed) {
                p = cpy(t, "no numeral: ");
                cpy(p, a);
                work_add(w, t, " assumed", NULL, NULL);
            }
            p = cpy(t, "cross charges -> ");
            p = put_uint(p, c->nc);
            *p++ = ':';
            put_uint(p, c->na);
            work_add(w, t, NULL, NULL, NULL);
        } else if (c->c.pi < 0 && variable(c->c.z)) {
            /* Fe2O3: 3 x 2 / 2 = 3+ -> (III) */
            char t[32], *p = t;
            p = cpy(p, elements[c->c.z].sym);
            p = cpy(p, " = ");
            p = put_uint(p, c->na);
            *p++ = 'x';
            p = put_uint(p, (unsigned)-c->a.q);
            *p++ = '/';
            p = put_uint(p, c->nc);
            p = cpy(p, " = ");
            p = put_charge(p, c->c.q);
            p = cpy(p, " (");
            p = cpy(p, roman[c->c.q - 1]);
            cpy(p, ")");
            work_add(w, t, NULL, NULL, NULL);
        } else if (c->c.pi < 0 && c->c.q != first_pos(c->c.z)) {
            char t[8];
            put_charge(cpy(t, " "), first_pos(c->c.z));
            work_add(w, "check: ", elements[c->c.z].sym, " is always", t);
        }
        {
            char t[32], u[16];
            t[0] = u[0] = 0;
            if (c->nc > 1) cpy(put_uint(t, c->nc), " ");
            strcat(t, a);
            if (c->na > 1) cpy(put_uint(u, c->na), " ");
            strcat(u, b);
            if (strlen(t) + strlen(u) + 9 <= WORK_COLS) work_add(w, "ions: ", t, " + ", u);
            else {
                work_add(w, "ions: ", t, NULL, NULL);
                work_add(w, "    + ", u, NULL, NULL);
            }
        }
        strcpy(row, "ionic");
        if (base) {
            add_tag(row, ", ", "strong base");
            work_add(w, "strong base: gives OH-", NULL, NULL, NULL);
        }
        if (s == EL_SALT || base) {
            if (!base) {
                char t[32];
                strcpy(t, a); strcat(t, " "); strcat(t, b);
                if (strlen(row) + 11 + strlen(t) <= UI_COLS) { strcat(row, ", soluble: "); strcat(row, t); }
                else if (strlen(row) + 8 + strlen(t) <= UI_COLS) { strcat(row, ", sol.: "); strcat(row, t); }
                else add_tag(row, ", ", "soluble");
            }
            work_add(w, "soluble in water", NULL, NULL, NULL);
            work_add(w, "strong electrolyte", NULL, NULL, NULL);
        } else {
            add_tag(row, ", ", "insoluble");
            work_add(w, "insoluble: precipitate", NULL, NULL, NULL);
        }
        break;
    }
    case NK_ACID: {
        bool strong = el_class(c, fa) == EL_STRONG_ACID;
        put_ion(b, &c->a);
        formula_pretty(b, b);
        strcpy(row, strong ? "strong acid" : "weak acid");
        if (strong) {
            char t[32];
            strcpy(t, "H+ ");
            strcat(t, b);
            add_tag(row, ": ", t);
        }
        work_add(w, "acid: H+ and ", b, NULL, NULL);
        if (c->a.pi < 0) work_add(w, "-ide -> hydro-ic acid", NULL, NULL, NULL);
        else {
            const char *n = ion((uint8_t)c->a.pi)->name;
            const char *e = n + strlen(n) - 3;
            work_add(w, "-", e, !strcmp(e, "ite") ? " -> -ous acid" : !strcmp(e, "ate") ? " -> -ic acid"
                                                                            : " -> hydro-ic acid", NULL);
        }
        work_add(w, strong ? "strong electrolyte" : "weak electrolyte", NULL, NULL, NULL);
        if (c->a.pi < 0 || strcmp(ion((uint8_t)c->a.pi)->name, "azide")) {
            char t[32];
            put_anion_name(cpy(t, "hydrogen "), &c->a);
            if (c->nc == 1 && c->a.pi < 0) work_add(w, "gas: ", t, NULL, NULL);
        }
        break;
    }
    case NK_COMMON: {
        char cls = common[c->cm - 1].cls;
        if (cls == 'b') {
            strcpy(row, "weak base");
            work_add(w, "weak base", NULL, NULL, NULL);
            work_add(w, "weak electrolyte", NULL, NULL, NULL);
        } else if (cls == 'a') {
            strcpy(row, "weak acid");
            work_add(w, "weak acid", NULL, NULL, NULL);
            work_add(w, "weak electrolyte", NULL, NULL, NULL);
        } else if (cls == 'i') {
            strcpy(row, "ionic");
        } else {
            strcpy(row, "covalent");
            work_add(w, "covalent, nonelectrolyte", NULL, NULL, NULL);
            if (!shp) add_tag(row, ", ", "nonelectrolyte");
        }
        if (shp) add_tag(row, ", ", sh.name);
        break;
    }
    case NK_COV:
        strcpy(row, "covalent");
        work_add(w, "covalent: 2 nonmetals", NULL, NULL, NULL);
        work_add(w, "prefix = atom count", NULL, NULL, NULL);
        work_add(w, "nonelectrolyte", NULL, NULL, NULL);
        add_tag(row, ", ", shp ? sh.name : "nonelectrolyte");
        break;
    case NK_ELEM: {
        static const char *const cat[] = {"metal", "nonmetal", "metalloid", "noble gas"};
        uint8_t z = c->c.z, i;
        strcpy(row, cat[elements[z].cat]);
        if (diatomic(z)) add_tag(row, ", ", "diatomic");
        a[0] = 0;
        for (i = 0; i < 3 && elements[z].ions[i]; i++) {
            char t[6];
            int8_t q = elements[z].ions[i];
            char *p = cpy(t, " ");
            if (q == 1 || q == -1) *p++ = '1';
            put_charge(p, q);
            strcat(a, t);
        }
        if (a[0]) {
            if (strlen(row) + 5 + strlen(a) <= UI_COLS) { strcat(row, ", ion"); strcat(row, a); }
            work_add(w, "usual ions:", a, NULL, NULL);
        }
        shp = false;
        break;
    }
    case NK_ION:
        strcpy(row, c->c.pi >= 0 ? "polyatomic" : "monatomic");
        strcat(row, c->c.q > 0 ? " cation" : " anion");
        if (shp && c->c.pi >= 0) { strcpy(row, "polyatomic"); add_tag(row, ", ", sh.name); }
        break;
    }
    if (shp) {
        uint8_t i;
        bool same = true;
        work_add(w, "shape: ", sh.name, NULL, NULL);
        if (sh.groups) {
            char t[32], *p = put_uint(t, sh.groups);
            p = cpy(p, " e- groups, ");
            p = put_uint(p, sh.lp);
            cpy(p, sh.lp == 1 ? " lone pair" : " lone pairs");
            work_add(w, t, NULL, NULL, NULL);
        }
        for (i = 1; i < f->nel; i++) if (f->el[i].z != f->el[0].z) same = false;
        work_add(w, "angle ", sh.angle,
                 c->kind == NK_ION ? NULL : (sh.polar && !same) ? ", polar" : ", nonpolar", NULL);
    }
    put_mass(f, w);
}

/* --------------------------------------------------------------- app */

static void finish(const cmpd_t *c, const char *main_, const char *fa, const formula_t *f,
                   char *out, work_t *w) {
    char row[UI_COLS + 8];
    info(c, fa, f, row, w);
    strncpy(out, main_, UI_OUT - 1);
    out[UI_OUT - 1] = 0;
    if (strlen(out) > UI_COLS) {                 /* long name: two rows */
        uint8_t k;
        for (k = UI_COLS; k && out[k] != ' '; k--) ;
        if (k) out[k] = '\n';
    }
    if (row[0] && strlen(out) + 1 + strlen(row) < UI_OUT) {
        strcat(out, "\n");
        strcat(out, row);
    }
}

/* name, wrapped onto two work lines */
static void work_name(work_t *w, const char *n) {
    uint8_t l = (uint8_t)strlen(n), k = l;
    if (l > WORK_COLS) {
        char h[WORK_COLS + 1];
        for (k = WORK_COLS; k && n[k] != ' '; k--) ;
        if (!k) k = WORK_COLS;
        memcpy(h, n, k);
        h[k] = 0;
        work_add(w, h, NULL, NULL, NULL);
        work_add(w, n + k + (n[k] == ' '), NULL, NULL, NULL);
    } else work_add(w, n, NULL, NULL, NULL);
}

void names_compute(const char *in, char *out, char *small, work_t *w) {
    char buf[ENTRY_MAX + 1], name[48], fa[48], pretty[48];
    cmpd_t c;
    formula_t f;
    uint8_t i, l = 0;
    out[0] = small[0] = 0;

    /* name -> formula */
    for (i = 0; in[i] && l < ENTRY_MAX; i++) {
        char ch = in[i];
        if (ch >= 'A' && ch <= 'Z') ch = (char)(ch + 32);
        if (ch == ',') ch = ' ';
        if (ch == ' ' && l && buf[l - 1] == ' ') continue;
        buf[l++] = ch;
    }
    buf[l] = 0;
    if (name_parse(buf, &c)) {
        c.from_name = true;
        cmpd_formula(&c, fa);
        memset(&f, 0, sizeof f);
        count(fa, fa + strlen(fa), 1, &f);
        if (c.hyd) {
            uint8_t k;
            for (k = 1; k <= 8; k += 7) {
                uint8_t j;
                for (j = 0; j < f.nel && f.el[j].z != k; j++) ;
                if (j == f.nel) { f.el[j].z = k; f.el[j].n = 0; f.nel++; }
                f.el[j].n = (uint16_t)(f.el[j].n + c.hyd * (k == 1 ? 2 : 1));
            }
        }
        formula_pretty(fa, pretty);
        cmpd_name(&c, name);
        work_add(w, "name -> formula", NULL, NULL, NULL);
        work_name(w, name);
        work_add(w, "= ", pretty, NULL, NULL);
        finish(&c, pretty, fa, &f, out, w);
        return;
    }

    /* formula -> name */
    if (!formula_parse(in, (uint8_t)strlen(in), FP_COMPOUND, &f)) {
        strcpy(out, "not recognized");
        return;
    }
    {
        const char *d = f.disp;
        const char *dot = strchr(d, CH_DOT);
        uint8_t len = (uint8_t)(dot ? (size_t)(dot - d) : strlen(d)), k;
        const char *also = NULL;
        memset(&c, 0, sizeof c);
        c.c.pi = c.a.pi = -1;
        for (k = 0; k < NCOMMON && !f.charge; k++) {
            if (word_eq(d, len, common[k].f)) {
                if (common[k].cls >= 'a') { c.kind = NK_COMMON; c.cm = (uint8_t)(k + 1); }
                else also = common[k].name;
                break;
            }
        }
        if (c.kind) ;
        else if (f.charge) {
            int8_t pi = ion_find(d, len, f.charge);
            if (f.nel == 1 && f.el[0].n == 1) {
                c.c.z = f.el[0].z;
                if (f.charge < 0 && root_of(c.c.z) < 0) c.kind = NK_NONE;
                else c.kind = NK_ION;
            } else if (pi >= 0) {
                c.c.pi = pi;
                c.kind = NK_ION;
            }
            c.c.q = f.charge;
        } else if (f.nel == 1 && f.npart == 1) {
            c.c.z = f.el[0].z;
            c.kind = NK_ELEM;
        } else {
            int8_t pi = ion_find(d, len, 0);
            bool has_h = formula_count(&f, 1) > 0;
            if (pi >= 0 && has_h && !(len > 1 && d[0] == 'H' && d[1] != 'g')) {
                /* oh, nh4: name the ion */
                c.c.pi = pi;
                c.c.q = ion((uint8_t)pi)->charge;
                c.kind = NK_ION;
            } else decompose(d, len, f.nel, &c);
            if (!c.kind && pi >= 0) {
                c.c.pi = pi;
                c.c.q = ion((uint8_t)pi)->charge;
                c.kind = NK_ION;
            }
        }
        if (!c.kind) {
            /* no naming rule (C2H5Cl, Fe3O4): still give what is known */
            char row[UI_COLS + 8];
            shape_t sh;
            formula_pretty(f.disp, pretty);
            work_add(w, pretty, NULL, NULL, NULL);
            strcpy(out, "no naming rule");
            strcpy(row, elem_is_metal(f.el[0].z) ? "charges don't balance" : "covalent");
            if (!elem_is_metal(f.el[0].z) && vsepr(&f, f.charge, &sh)) add_tag(row, ", ", sh.name);
            strcat(out, "\n");
            strcat(out, row);
            put_mass(&f, w);
            return;
        }
        c.hyd = f.hydrate;
        cmpd_name(&c, name);
        cmpd_formula(&c, fa);
        formula_pretty(f.disp, pretty);
        work_add(w, pretty, " -> name", NULL, NULL);
        work_name(w, name);
        if (also) work_add(w, "also: ", also, NULL, NULL);
        if (c.kind == NK_COV) {                     /* clo3, so4: say the ion too */
            int8_t pi = ion_find(d, len, -1);
            if (pi >= 0) {
                part_t x;
                char t[16];
                x.pi = pi; x.z = 0; x.q = ion((uint8_t)pi)->charge;
                put_ion(t, &x);
                formula_pretty(t, t);
                work_add(w, "ion ", t, ": ", ion((uint8_t)pi)->name);
            }
        }
        if (f.alt_z) {
            char *p = cpy(small, elements[f.alt_z].sym);
            p = cpy(p, ": ");
            cpy(p, elements[f.alt_z].name);
            work_add(w, "or element ", small, NULL, NULL);
        }
        finish(&c, name, fa, &f, out, w);
    }
}
