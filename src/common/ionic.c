/* ionic.c - ions, solubility rules, electrolyte class */
#include <string.h>
#include "ionic.h"

static bool upper(char c) { return c >= 'A' && c <= 'Z'; }
static bool lowc(char c) { return c >= 'a' && c <= 'z'; }
static bool digit(char c) { return c >= '0' && c <= '9'; }

bool ion_alkali(uint8_t z) { return z == 3 || z == 11 || z == 19 || z == 37 || z == 55; }
bool ion_heavy2(uint8_t z, bool mg) { return z == 20 || z == 38 || z == 56 || (mg && z == 12); }

/* element symbol at s: length 1-2, 0 if none */
static uint8_t sym_len(const char *s) {
    if (!upper(s[0])) return 0;
    return (uint8_t)(lowc(s[1]) ? 2 : 1);
}

static uint8_t read_num(const char **p) {
    uint8_t v = 0;
    if (!digit(**p)) return 1;
    while (digit(**p)) v = (uint8_t)(v * 10 + (*(*p)++ - '0'));
    return v;
}

static void copyn(char *d, const char *s, uint8_t n) {
    if (n > 6) n = 6;
    memcpy(d, s, n);
    d[n] = 0;
}

/* anion text r[0..len): polyatomic, "(X)n", or one element + count */
static bool anion(const char *r, uint8_t len, bool peroxide_ok, ionic_t *o) {
    int k;
    const char *p;
    if (!len) return false;
    if (r[0] == '(') {
        const char *e = memchr(r, ')', len);
        if (!e) return false;
        k = polyion_find(r + 1, (uint8_t)(e - r - 1));
        p = e + 1;
        o->na = read_num(&p);
        if (k < 0 || polyions[k].charge >= 0 || p != r + len) return false;
    } else {
        k = polyion_find(r, len);
        if (k >= 0 && (polyions[k].charge >= 0 || (!strcmp(polyions[k].formula, "O2") && !peroxide_ok)))
            k = -1;
        if (k >= 0) {
            o->na = 1;
        } else {
            uint8_t sl = sym_len(r), z;
            p = r + sl;
            if (!sl || !(z = elem_find(r, sl)) || elements[z].ions[0] >= 0) return false;
            o->na = read_num(&p);
            if (p != r + len) return false;
            copyn(o->an, r, sl);
            o->za = z;
            o->qa = elements[z].ions[0];
            return true;
        }
    }
    copyn(o->an, polyions[k].formula, (uint8_t)strlen(polyions[k].formula));
    o->qa = polyions[k].charge;
    return true;
}

bool ionic_split(const formula_t *f, ionic_t *o) {
    const char *d = f->disp, *p, *end = strchr(d, CH_DOT);
    uint8_t len = (uint8_t)(end ? (size_t)(end - d) : strlen(d)), sl;
    int neg;
    memset(o, 0, sizeof *o);
    if (f->nel < 2 || f->charge) return false;
    if (d[0] == 'H' && !lowc(d[1]) && strcmp(d, "H2O") && strcmp(d, "H2O2")) {
        p = d + 1;                                  /* acid: H first */
        o->kind = IK_ACID;
        strcpy(o->cat, "H");
        o->zc = 1;
        o->nc = read_num(&p);
    } else if (!strncmp(d, "NH4", 3) && !digit(d[3])) {
        o->kind = IK_IONIC;
        strcpy(o->cat, "NH4");
        o->nc = 1;
        p = d + 3;
    } else if (d[0] == '(') {
        const char *e = strchr(d, ')');
        int k = e ? polyion_find(d + 1, (uint8_t)(e - d - 1)) : -1;
        if (k < 0 || polyions[k].charge <= 0) return false;
        o->kind = IK_IONIC;
        copyn(o->cat, d + 1, (uint8_t)(e - d - 1));
        p = e + 1;
        o->nc = read_num(&p);
    } else {
        sl = sym_len(d);
        o->zc = elem_find(d, sl);
        if (!o->zc || !elem_is_metal(o->zc)) return false;
        o->kind = IK_IONIC;
        copyn(o->cat, d, sl);
        p = d + sl;
        o->nc = read_num(&p);
    }
    if (!anion(p, (uint8_t)(d + len - p), ion_alkali(o->zc) || ion_heavy2(o->zc, true), o)) {
        o->kind = IK_NONE;
        return false;
    }
    neg = -o->qa * o->na;
    if (!o->nc || neg % o->nc || neg / o->nc < 1 || neg / o->nc > 7) {
        o->kind = IK_NONE;
        return false;
    }
    o->qc = (int8_t)(neg / o->nc);
    if (o->kind == IK_ACID && o->qc != 1) { o->kind = IK_NONE; return false; }
    return true;
}

static bool is(const char *a, const char *b) { return !strcmp(a, b); }

bool ionic_soluble(const ionic_t *o) {
    const char *a = o->an;
    uint8_t z = o->zc;
    bool g1 = ion_alkali(z) || is(o->cat, "NH4");
    if (o->kind == IK_ACID || o->kind == IK_NONE || g1) return true;
    if (is(a, "NO3") || is(a, "C2H3O2") || is(a, "CH3CO2") || is(a, "ClO4") || is(a, "ClO3"))
        return true;
    if (is(a, "Cl") || is(a, "Br") || is(a, "I"))
        return !(z == 47 || z == 82 || is(o->cat, "Hg2") || ((z == 80 || z == 29) && o->qc == 1));
    if (is(a, "SO4"))
        return !(z == 47 || z == 82 || ion_heavy2(z, false) || is(o->cat, "Hg2") || (z == 80 && o->qc == 1));
    if (is(a, "F")) return !(z == 82 || ion_heavy2(z, true));
    if (is(a, "OH") || is(a, "O")) return ion_heavy2(z, false);
    if (is(a, "S")) return ion_heavy2(z, true);
    if (is(a, "CO3") || is(a, "PO4") || is(a, "SO3") || is(a, "CrO4") || is(a, "C2O4") ||
        is(a, "PO3") || is(a, "HPO4"))
        return false;
    return true;
}

uint8_t ionic_class(const formula_t *f, const ionic_t *o) {
    static const char *const strong[] = {"HCl", "HBr", "HI", "HNO3", "H2SO4", "HClO4", "HClO3"};
    uint8_t i;
    if (o->kind == IK_ACID) {
        for (i = 0; i < 7; i++) if (is(f->disp, strong[i])) return EL_STRONG_ACID;
        return EL_WEAK_ACID;
    }
    if (o->kind == IK_IONIC) {
        if (!ionic_soluble(o)) return EL_INSOLUBLE;
        return is(o->an, "OH") ? EL_STRONG_BASE : EL_SALT;
    }
    if (is(f->disp, "NH3")) return EL_WEAK_BASE;
    /* acids written with the H last: HC2H3O2 as CH3COOH */
    if (is(f->disp, "CH3COOH")) return EL_WEAK_ACID;
    return EL_NONELEC;
}

uint8_t ion_str(const char *sym, int8_t q, char *buf) {
    char *p = buf;
    uint8_t a = (uint8_t)(q < 0 ? -q : q);
    while (*sym) *p++ = *sym++;
    if (a > 1) {
        if (digit(p[-1])) *p++ = ' ';
        *p++ = (char)('0' + a);
    }
    if (q) *p++ = q > 0 ? '+' : '-';
    *p = 0;
    return (uint8_t)(p - buf);
}
