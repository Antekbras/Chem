/* formula.c - chemical formula parser with auto-case */
#include <string.h>
#include "formula.h"
#include "num.h"

enum { T_RUN, T_NUM, T_LP, T_RP, T_DOT };

typedef struct {
    uint8_t type, start, len;
    uint16_t val;
} ftok_t;

#define MAXTOK 32
#define MAXSEG 12           /* splits kept per letter run */
#define MAXCOMBO 2048

typedef struct {
    ftok_t tok[MAXTOK];
    uint8_t ntok;
    uint16_t seg[MAXTOK][MAXSEG];  /* bit i set: a symbol starts at run char i */
    uint8_t nseg[MAXTOK];
    uint8_t pick[MAXTOK];
    const char *s;
} fstate_t;

static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static bool isletter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool isnum(char c) { return c >= '0' && c <= '9'; }

static void split_run(fstate_t *st, uint8_t ti, uint8_t pos, uint16_t mask) {
    const ftok_t *t = &st->tok[ti];
    const char *r = st->s + t->start;
    uint8_t k;
    if (pos == t->len) {
        if (st->nseg[ti] < MAXSEG) st->seg[ti][st->nseg[ti]++] = mask;
        return;
    }
    /* two-letter symbols first */
    for (k = 2; k >= 1; k--) {
        if (pos + k <= t->len && elem_find(r + pos, k))
            split_run(st, ti, (uint8_t)(pos + k), (uint16_t)(mask | (1u << pos)));
    }
}

/* walk the symbols of the current pick; returns count, fills z[] and after[]
 * (index of the token that follows each symbol's run, or 0xFF inside a run) */
static uint8_t symbols(const fstate_t *st, uint8_t *z, uint8_t *tokidx, uint8_t *last_in_run) {
    uint8_t n = 0, i, j;
    for (i = 0; i < st->ntok; i++) {
        const ftok_t *t = &st->tok[i];
        uint16_t m;
        if (t->type != T_RUN) continue;
        m = st->seg[i][st->pick[i]];
        for (j = 0; j < t->len; j++) {
            if (!(m & (1u << j))) continue;
            {
                uint8_t k = (uint8_t)(j + 1 < t->len && !(m & (1u << (j + 1))) ? 2 : 1);
                z[n] = elem_find(st->s + t->start + j, k);
                tokidx[n] = i;
                last_in_run[n] = (uint8_t)(j + k == t->len);
                n++;
            }
        }
    }
    return n;
}

static bool molecular_element(uint8_t z, uint16_t sub) {
    switch (z) {
    case 1: case 7: case 8: case 9: case 17: case 35: case 53: return sub == 2 || (z == 8 && sub == 3);
    case 15: return sub == 4;
    case 16: return sub == 8;
    }
    return false;
}

static int score(const fstate_t *st) {
    uint8_t z[48], ti[48], lr[48], n, i;
    int sc = 0;
    bool before_dot = true;
    n = symbols(st, z, ti, lr);
    for (i = 0; i < n; i++) {
        static const uint8_t rar[4] = {0, 3, 6, 15};
        sc += 10 + rar[elem_rarity(z[i])];
    }
    /* a lone element with a subscript must be a real molecule (O2, S8) */
    if (n == 1) {
        uint8_t k = (uint8_t)(ti[0] + 1);
        if (k < st->ntok && st->tok[k].type == T_NUM && !molecular_element(z[0], st->tok[k].val))
            sc += 20;
    }
    for (i = 0; i < n; i++) {
        uint8_t k;
        /* hydrate dot between symbols resets the checks */
        for (k = (uint8_t)(i ? ti[i - 1] : 0); i && k < ti[i]; k++)
            if (st->tok[k].type == T_DOT) before_dot = false;
        if (i && before_dot && !elem_is_metal(z[i - 1]) && elem_is_metal(z[i]))
            sc += 12;                       /* metals come first: H2CO3 not H2Co3 */
    }
    if (n > 1) {
        /* a compound ends in a nonmetal: Na2CO3 not Na2Co3 */
        uint8_t last = (uint8_t)(n - 1);
        for (i = 0; i < n; i++) {
            uint8_t k;
            bool dot = false;
            for (k = 0; k < ti[i]; k++) if (st->tok[k].type == T_DOT) dot = true;
            if (!dot) last = i;
        }
        if (last > 0 && elem_is_metal(z[last])) sc += 12;
    }
    return sc;
}

static void add_count(formula_t *f, uint8_t z, uint16_t n) {
    uint8_t i;
    for (i = 0; i < f->nel; i++) if (f->el[i].z == z) { f->el[i].n += n; return; }
    if (f->nel < F_MAXEL) { f->el[f->nel].z = z; f->el[f->nel].n = n; f->nel++; }
}

/* "no3-" is nitrate, not nobelium 3-: letters+digits spell a polyatomic ion */
static bool polyion_like(const char *s, uint8_t len, int8_t sign) {
    uint8_t i, k;
    for (i = 0; i < NUM_POLYIONS; i++) {
        const char *pf = polyions[i].formula;
        if (polyions[i].charge != sign) continue;   /* no3- yes, o2- is oxide */
        for (k = 0; k < len && pf[k] && lower(pf[k]) == lower(s[k]); k++) ;
        if (k == len && !pf[k]) return true;
    }
    return false;
}

static const char *const ambiguous[] = {"co", "cs", "hf", "no"};

bool formula_parse(const char *s, uint8_t len, uint8_t prefer, formula_t *f) {
    fstate_t st;
    uint8_t i, n, end = len, nsym;
    uint32_t combos = 1, c, bestc = 0;
    int best = 0x7fff;
    uint16_t mult[MAXTOK];
    uint8_t z[48], ti[48], lr[48];

    memset(f, 0, sizeof *f);
    memset(&st, 0, sizeof st);
    f->coef = 1;
    st.s = s;
    while (end && s[end - 1] == ' ') end--;
    while (len && *s == ' ') { s++; len--; end--; st.s = s; }
    if (!end) return false;

    /* ---- trailing charge: "3+", "2-", "+", " 2-" ---- */
    if (s[end - 1] == '+' || s[end - 1] == '-' || s[end - 1] == CH_NEG) {
        int8_t sign = s[end - 1] == '+' ? 1 : -1;
        uint8_t d = (uint8_t)(end - 1), k = 0;
        while (d > 0 && isnum(s[d - 1])) { d--; k++; }
        if (k == 0) {
            f->charge = sign;
            end--;
        } else if (d > 0 && s[d - 1] == ' ') {
            f->charge = (int8_t)(sign * (s[end - 2] - '0'));  /* "so4 2-" */
            end = (uint8_t)(d - 1);
        } else {
            /* single element: all digits are the charge (fe3+), unless > 7 (hg22+) */
            uint8_t letters = 0, q;
            bool other = false;
            for (q = 0; q < d; q++) {
                if (isletter(s[q])) letters++;
                else other = true;
            }
            if (!other && letters <= 2 && elem_find(s, letters) && k == 1 &&
                !polyion_like(s, end - 1, sign)) {
                f->charge = (int8_t)(sign * (s[end - 2] - '0'));
                end = d;
                prefer = FP_ELEMENT;         /* co2+ is cobalt(II) */
            } else if (k >= 2) {
                f->charge = (int8_t)(sign * (s[end - 2] - '0'));
                end = (uint8_t)(end - 2);
            } else {
                f->charge = sign;            /* no3- : digits are subscripts */
                end--;
            }
        }
        while (end && s[end - 1] == ' ') end--;
    }

    /* ---- leading coefficient ---- */
    i = 0;
    if (isnum(s[0])) {
        uint16_t v = 0;
        while (i < end && isnum(s[i])) v = (uint16_t)(v * 10 + (s[i++] - '0'));
        if (!v || v > 255) return false;
        f->coef = (uint8_t)v;
    }

    /* ---- tokens ---- */
    for (; i < end;) {
        ftok_t *t = &st.tok[st.ntok];
        char ch = s[i];
        if (st.ntok >= MAXTOK) return false;
        t->start = i;
        if (isletter(ch)) {
            t->type = T_RUN;
            while (i < end && isletter(s[i])) i++;
            if (i - t->start > 14) return false;
        } else if (isnum(ch)) {
            t->type = T_NUM;
            while (i < end && isnum(s[i])) t->val = (uint16_t)(t->val * 10 + (s[i++] - '0'));
            if (!t->val) return false;
        } else if (ch == '(' || ch == '[') { t->type = T_LP; i++; }
        else if (ch == ')' || ch == ']') { t->type = T_RP; i++; }
        else if (ch == '.' || ch == CH_DOT || ch == '*') { t->type = T_DOT; i++; }
        else return false;
        t->len = (uint8_t)(i - t->start);
        st.ntok++;
    }
    if (!st.ntok) return false;

    /* ---- every split of every letter run ---- */
    for (i = 0; i < st.ntok; i++) {
        if (st.tok[i].type != T_RUN) continue;
        split_run(&st, i, 0, 0);
        if (!st.nseg[i]) return false;
        combos *= st.nseg[i];
        if (combos > MAXCOMBO) return false;
    }

    /* ambiguous two-letter inputs */
    if (st.ntok == 1 && st.tok[0].len == 2) {
        uint8_t a;
        for (a = 0; a < sizeof ambiguous / sizeof ambiguous[0]; a++)
            if (lower(s[0]) == ambiguous[a][0] && lower(s[1]) == ambiguous[a][1]) break;
        if (a < sizeof ambiguous / sizeof ambiguous[0] || prefer == FP_ELEMENT) {
            uint8_t ez = elem_find(s, 2), k;
            for (k = 0; k < st.nseg[0]; k++) {
                bool whole = st.seg[0][k] == 1;      /* one symbol */
                if (prefer == FP_ELEMENT ? whole : !whole) { st.pick[0] = k; break; }
            }
            if (k < st.nseg[0]) {
                if (prefer == FP_COMPOUND && ez && st.seg[0][k] != 1) f->alt_z = ez;
                goto chosen;
            }
        }
    }

    for (c = 0; c < combos; c++) {
        uint32_t r = c;
        int sc;
        for (i = 0; i < st.ntok; i++) {
            if (st.tok[i].type != T_RUN) continue;
            st.pick[i] = (uint8_t)(r % st.nseg[i]);
            r /= st.nseg[i];
        }
        sc = score(&st);
        if (sc < best) { best = sc; bestc = c; }
    }
    {
        uint32_t r = bestc;
        for (i = 0; i < st.ntok; i++) {
            if (st.tok[i].type != T_RUN) continue;
            st.pick[i] = (uint8_t)(r % st.nseg[i]);
            r /= st.nseg[i];
        }
    }
chosen:
    /* ---- multipliers: subscripts, groups, hydrate coefficients ---- */
    {
        uint8_t stack[6], sp = 0;
        uint16_t outer = 1;           /* hydrate coefficient */
        for (i = 0; i < st.ntok; i++) mult[i] = 1;
        for (i = 0; i < st.ntok; i++) {
            ftok_t *t = &st.tok[i];
            if (t->type == T_LP) {
                if (sp >= 6) return false;
                stack[sp++] = i;
            } else if (t->type == T_RP) {
                uint8_t open, k;
                uint16_t m = 1;
                if (!sp) return false;
                open = stack[--sp];
                if (i + 1 < st.ntok && st.tok[i + 1].type == T_NUM) m = st.tok[i + 1].val;
                for (k = (uint8_t)(open + 1); k < i; k++) mult[k] = (uint16_t)(mult[k] * m);
            } else if (t->type == T_DOT) {
                outer = 1;
                if (i + 1 < st.ntok && st.tok[i + 1].type == T_NUM) outer = st.tok[i + 1].val;
                {
                    uint8_t k;
                    for (k = (uint8_t)(i + 1); k < st.ntok; k++) mult[k] = (uint16_t)(mult[k] * outer);
                }
            }
        }
        if (sp) return false;
    }

    /* ---- counts and canonical text ---- */
    n = symbols(&st, z, ti, lr);
    nsym = n;
    {
        uint8_t k;
        for (k = 0; k < n; k++) {
            uint16_t sub = 1;
            if (!z[k]) return false;
            if (lr[k] && ti[k] + 1 < st.ntok && st.tok[ti[k] + 1].type == T_NUM)
                sub = st.tok[ti[k] + 1].val;
            add_count(f, z[k], (uint16_t)(sub * mult[ti[k]]));
        }
    }
    {
        char *p = f->disp;
        uint8_t k = 0, depth = 0;
        bool hyd = false;
        for (i = 0; i < st.ntok; i++) {
            ftok_t *t = &st.tok[i];
            uint8_t before = (uint8_t)(p - f->disp);
            if (p - f->disp > F_DISP - 8) return false;
            switch (t->type) {
            case T_RUN:
                while (k < n && ti[k] == i) {
                    const char *sym = elements[z[k]].sym;
                    uint8_t pstart = (uint8_t)(p - f->disp);
                    *p++ = sym[0];
                    if (sym[1]) *p++ = sym[1];
                    if (!hyd && depth == 0 && f->npart < F_MAXPART) {
                        fpart_t *pt = &f->part[f->npart++];
                        pt->start = pstart; pt->len = (uint8_t)(p - f->disp - pstart);
                        pt->mult = 1; pt->group = false;
                    }
                    k++;
                }
                break;
            case T_NUM: {
                char d[6];
                int8_t q = 0;
                uint16_t v = t->val;
                do { d[q++] = (char)('0' + v % 10); v /= 10; } while (v);
                while (q) *p++ = d[--q];
                /* subscript of the previous top-level part */
                if (!hyd && depth == 0 && f->npart && i > 0 &&
                    (st.tok[i - 1].type == T_RUN || st.tok[i - 1].type == T_RP)) {
                    f->part[f->npart - 1].mult = (uint8_t)t->val;
                    f->part[f->npart - 1].len = (uint8_t)(p - f->disp - f->part[f->npart - 1].start);
                }
                if (i > 0 && st.tok[i - 1].type == T_DOT) f->hydrate = (uint8_t)t->val;
                break;
            }
            case T_LP:
                if (!hyd && depth == 0 && f->npart < F_MAXPART) {
                    fpart_t *pt = &f->part[f->npart++];
                    pt->start = before; pt->len = 0; pt->mult = 1; pt->group = true;
                }
                depth++;
                *p++ = '(';
                break;
            case T_RP:
                depth--;
                *p++ = ')';
                if (!hyd && depth == 0 && f->npart)
                    f->part[f->npart - 1].len = (uint8_t)(p - f->disp - f->part[f->npart - 1].start);
                break;
            case T_DOT:
                hyd = true;
                *p++ = CH_DOT;
                if (!(i + 1 < st.ntok && st.tok[i + 1].type == T_NUM)) f->hydrate = 1;
                break;
            }
        }
        *p = 0;
    }
    /* hydrate count only when the hydrate part is water */
    if (f->hydrate) {
        char *d = strchr(f->disp, CH_DOT);
        uint8_t q = 1;
        while (d && isnum(d[q])) q++;
        if (!d || strcmp(d + q, "H2O")) f->hydrate = 0;
    }
    f->nsym = nsym;
    return true;
}

fp_t formula_mass(const formula_t *f) {
    fp_t m = 0;
    uint8_t i;
    for (i = 0; i < f->nel; i++) m += elem_mass(f->el[i].z) * f->el[i].n;
    return m;
}

uint16_t formula_count(const formula_t *f, uint8_t z) {
    uint8_t i;
    for (i = 0; i < f->nel; i++) if (f->el[i].z == z) return f->el[i].n;
    return 0;
}

static char *put_uint(char *p, unsigned v) {
    char d[6];
    int q = 0;
    do { d[q++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (q) *p++ = d[--q];
    return p;
}

uint8_t formula_str(const formula_t *f, char *buf) {
    char *p = buf;
    const char *d = f->disp;
    while (*d) *p++ = *d++;
    if (f->charge) {
        int c = f->charge < 0 ? -f->charge : f->charge;
        if (c > 1 && p > buf && isnum(p[-1])) *p++ = ' ';
        if (c > 1) p = put_uint(p, (unsigned)c);
        *p++ = f->charge > 0 ? '+' : '-';
    }
    *p = 0;
    return (uint8_t)(p - buf);
}

uint8_t formula_work_line(const formula_t *f, uint8_t i, char *buf) {
    char *p = buf, num[24];
    const char *sym = elements[f->el[i].z].sym;
    num_t m = num_exact(elem_mass(f->el[i].z));
    num_t tot = num_exact(elem_mass(f->el[i].z) * f->el[i].n);
    *p++ = sym[0];
    *p++ = sym[1] ? sym[1] : ' ';
    *p++ = ' ';
    p = put_uint(p, f->el[i].n);
    *p++ = '*';
    num_fmt_plain(m, num); strcpy(p, num); p += strlen(num);
    *p++ = '=';
    num_fmt_plain(tot, num); strcpy(p, num); p += strlen(num);
    *p = 0;
    return (uint8_t)(p - buf);
}

uint8_t formula_pretty(const char *a, char *out) {
    char *p = out;
    char prev = 0;
    bool sub = false;
    for (; *a; a++) {
        if (isnum(*a)) {
            if (isletter(prev) || prev == ')' || (sub && isnum(prev))) {
                *p++ = (char)(CH_SUB0 + (*a - '0'));
                sub = true;
                prev = *a;
                continue;
            }
        } else sub = false;
        *p++ = *a;
        prev = *a;
    }
    *p = 0;
    return (uint8_t)(p - out);
}
