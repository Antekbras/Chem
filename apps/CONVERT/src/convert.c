/* convert.c - CONVERT engine: units, g/mol/particles, molar mass,
 * molarity, dilution, % and element mass in a compound. */
#include <string.h>
#include "convert.h"
#include "units.h"
#include "formula.h"
#include "ui.h"
#include "app.h"

#define NA_VALUE 6.022e23
#define MAXQ 4

/* quantity kinds, in "chem base" units: mol, g, L, mol/L, g/mol */
enum { KD_NONE, KD_AMT, KD_MASS, KD_VOL, KD_CONC, KD_MM, KD_OTHER };
static const int8_t kdims[5][NDIM] = {
    {0, 0, 0, 1, 0}, {0, 1, 0, 0, 0}, {3, 0, 0, 0, 0}, {-3, 0, 0, 1, 0}, {0, 1, 0, -1, 0}};
static const fp_t kbase[5] = {1, 1e-3, 1e-3, 1e3, 1e-3};
static const char *const kname[5] = {"mol", "g", "L", "M", "g/mol"};

typedef struct {
    num_t n;
    unit_t u;
    uint8_t side;   /* 0 = given, 1 = after the arrow */
    bool num;       /* a number was typed (else "1 unit", exact) */
} cq_t;

static struct {
    cq_t q[MAXQ];
    uint8_t nq;
    formula_t f, el;
    bool hasf, hasel, haswant;
    unit_t want;
} c;

static const char *err;

const char *const subst[NSUBST] = {
    "CO2", "N2", "H2", "NaCl", "KCl", "HCl", "HNO3", "NaOH", "CH3COOH",
    "CaCO3", "NaHCO3", "CuSO4", "AgNO3", "KMnO4", "MgO", "C", "Al"};

const char *const unitlist[NUNITLIST] = {
    "g", "kg", "mg", "ug", "lb", "oz", "amu",
    "mol", "particles", "atoms", "molecules", "M", "g/mol",
    "L", "mL", "cm3", "m3", "gal", "qt",
    "m", "cm", "mm", "nm", "km", "in", "ft", "mi",
    "s", "min", "hr", "J", "kJ", "cal", "atm", "torr", "K"};

void convert_pick_text(const char *item, bool unit, char prev, char *out) {
    if (!unit && prev && prev != ' ' && prev != CH_STO) *out++ = ' ';
    for (; *item; item++)
        *out++ = (*item >= 'A' && *item <= 'Z' && !(unit && !strcmp(item, "M")))
                     ? (char)(*item + 32) : *item;
    *out = 0;
}

static uint8_t kind_of(const unit_t *u) {
    udim_t d;
    uint8_t k;
    if (!u->n) return KD_NONE;
    unit_dim(u, &d);
    for (k = 0; k < 5; k++)
        if (!memcmp(d.d, kdims[k], NDIM)) return (uint8_t)(k + 1);
    return KD_OTHER;
}

static bool is_sep(char ch) { return ch == ' ' || ch == CH_STO || ch == 0; }
static bool numstart(char ch) {
    return (ch >= '0' && ch <= '9') || ch == '.' || ch == CH_NEG || ch == '-';
}

/* whole item is a unit: 1 = any unit, 2 = a chemistry unit (or lone m) */
static uint8_t unit_item(const char *s, uint8_t len, unit_t *u) {
    uint8_t k;
    if (unit_parse(s, u) != len) return 0;
    k = kind_of(u);
    return (u->ambig || (k >= KD_AMT && k <= KD_MM)) ? 2 : 1;
}

/* every element of g is in the given substance */
static bool part_of(const formula_t *g) {
    uint8_t i;
    if (!c.hasf) return false;
    for (i = 0; i < g->nel; i++)
        if (!formula_count(&c.f, g->el[i].z)) return false;
    return true;
}

static cq_t *new_q(uint8_t side) {
    cq_t *q;
    if (c.nq >= MAXQ) return NULL;
    q = &c.q[c.nq++];
    memset(q, 0, sizeof *q);
    q->side = side;
    return q;
}

static bool parse(const char *s) {
    uint8_t side = 0;
    bool bare = false;          /* previous item: a number without a unit */
    memset(&c, 0, sizeof c);
    while (*s) {
        const char *e;
        uint8_t len;
        unit_t u;
        if (*s == ' ') { s++; continue; }
        if (*s == CH_STO) {
            if (side) return false;
            side = 1; bare = false; s++;
            continue;
        }
        for (e = s; !is_sep(*e); e++) ;
        len = (uint8_t)(e - s);
        if (numstart(*s)) {
            cq_t *q = new_q(side);
            uint8_t n, k = 0;
            if (!q) return false;
            n = num_parse(s, &q->n);
            if (!n || n > len) return false;
            q->num = true;
            if (n < len) k = unit_parse(s + n, &q->u);
            if (n + k < len) {                     /* 5.23E24co2 */
                if (side || c.hasf) return false;
                if (formula_parse(s + n, (uint8_t)(len - n), FP_COMPOUND, &c.f)) {
                    memset(&q->u, 0, sizeof q->u);
                } else if (!k || !formula_parse(s + n + k, (uint8_t)(len - n - k), FP_COMPOUND, &c.f)) {
                    return false;
                }
                c.hasf = true;
            }
            bare = !q->u.n && !c.hasf;
        } else if (bare && unit_item(s, len, &u) == 2) {
            c.q[c.nq - 1].u = u;                   /* "250.0 ml" */
            bare = false;
        } else if (!side) {
            bare = false;
            if (!c.hasf && formula_parse(s, len, FP_COMPOUND, &c.f)) {
                c.hasf = true;
            } else if (unit_item(s, len, &u)) {    /* "mi/hr→m/s": 1 mi/hr */
                cq_t *q = new_q(0);
                if (!q) return false;
                q->n = num_exact(1);
                q->u = u;
            } else return false;
        } else {
            formula_t g;
            uint8_t ui = unit_item(s, len, &u);
            bool isf = formula_parse(s, len, FP_COMPOUND, &g) && part_of(&g);
            if (ui && !c.haswant && (!isf || ui == 2)) {
                c.want = u;
                c.haswant = true;
            } else if (isf && !c.hasel) {
                c.el = g;
                c.hasel = true;
            } else return false;
        }
        s = e;
    }
    return true;
}

/* ---- numbers and work lines ---- */

static work_t *W;               /* show-work lines of this compute */
static num_t NA;

static fp_t round2(fp_t v) { return FP_ROUND(v * 100) / 100; }

/* r = a op b (op '*' or '/'), work: "lhs a op b" and " =r unit" */
static void calc(num_t *r, const char *lhs, const num_t *a, char op, const num_t *b, const char *unit) {
    char x[24], y[24], o[2];
    num_t t = op == '*' ? num_mul(*a, *b, false) : num_div(*a, *b, false);
    num_fmt_plain(*a, x);
    num_fmt_plain(*b, y);
    o[0] = op; o[1] = 0;
    work_add(W, lhs, x, o, y);
    *r = t;
    if (unit) {
        num_fmt_plain(t, x);
        work_add(W, " =", x, " ", unit);
    }
}

/* r = a * exact factor, no work line */
static void scale(num_t *r, const num_t *a, fp_t f) {
    if (f != 1) *r = num_mul(*a, num_exact(f), false);
    else *r = *a;
}

/* molar mass, 2 decimals (KCl = 74.55), with the breakdown */
static void molar(num_t *M, const formula_t *f, bool work) {
    fp_t m = round2(formula_mass(f));
    *M = num_with_sf(m, fp_exp10(m) + 3);
    if (work) {
        char line[40], pretty[48];
        uint8_t i;
        for (i = 0; i < f->nel; i++) {
            formula_work_line(f, i, line);
            work_add(W, line, NULL, NULL, NULL);
        }
        formula_pretty(f->disp, pretty);
        num_fmt_plain(*M, line);
        work_add(W, "M ", pretty, "=", line);
    }
}

static fp_t ufactor(const unit_t *u, uint8_t k) {
    udim_t d;
    unit_dim(u, &d);
    return d.f / kbase[k - 1];
}

static void to_base(num_t *r, const cq_t *q, uint8_t k) { scale(r, &q->n, ufactor(&q->u, k)); }

static void finish(const num_t *r, const char *unit, const char *what, char *out, char *small) {
    char line[96];
    num_fmt_answer(*r, line, small);
    if (unit[0]) { strcat(line, " "); strcat(line, unit); }
    if (what && what[0]) { strcat(line, " "); strcat(line, what); }
    strncpy(out, line, UI_OUT - 1);
    out[UI_OUT - 1] = 0;
}

static bool is_count_unit(const unit_t *u) {
    return u->n == 1 && u->t[0].id >= U_PARTICLE && u->t[0].id <= U_PARTICLE + 3;
}

/* r in base units of kind k -> unit u (NULL = the base unit); show it */
static void answer(num_t *r, uint8_t k, const unit_t *u, const char *what, char *out, char *small) {
    char ustr[24];
    if (u) {
        unit_str(u, ustr);
        if (is_count_unit(u)) calc(r, "N=n*NA=", r, '*', &NA, ustr);
        else scale(r, r, 1 / ufactor(u, k));
    } else strcpy(ustr, kname[k - 1]);
    finish(r, ustr, what, out, small);
}

/* ---- dilution: M1V1 = M2V2 ---- */
static bool dilution(char *out, char *small) {
    const cq_t *C[2] = {0, 0}, *V[2] = {0, 0}, *pc, *pv;
    uint8_t i, pair;
    num_t a, b, prod, r;
    for (i = 0; i < c.nq; i++) {
        const cq_t *q = &c.q[i];
        uint8_t k = kind_of(&q->u);
        if (k == KD_CONC && !C[q->side]) C[q->side] = q;
        else if (k == KD_VOL && !V[q->side]) V[q->side] = q;
        else return false;
    }
    if (C[0] && V[0] && (!C[1] != !V[1])) pair = 0;
    else if (C[1] && V[1] && (!C[0] != !V[0])) pair = 1;
    else return false;
    pc = C[pair];
    pv = V[pair];
    work_add(W, "M1V1=M2V2", NULL, NULL, NULL);
    to_base(&a, pc, KD_CONC);
    to_base(&b, pv, KD_VOL);
    calc(&prod, "MV=", &a, '*', &b, "mol");
    if (V[1 - pair]) {                  /* missing M */
        to_base(&a, V[1 - pair], KD_VOL);
        calc(&r, "M=", &prod, '/', &a, "M");
        answer(&r, KD_CONC, &pc->u, NULL, out, small);
    } else {                            /* missing V */
        to_base(&a, C[1 - pair], KD_CONC);
        calc(&r, "V=", &prod, '/', &a, "L");
        answer(&r, KD_VOL, &pv->u, NULL, out, small);
    }
    return true;
}

static bool chem(char *out, char *small) {
    num_t v[KD_MM + 1], M, n, r;
    bool has[KD_MM + 1], hasn = true, count = false;
    uint8_t i, k, wk;
    const unit_t *wu = c.haswant ? &c.want : NULL;
    unit_t tmp;
    memset(has, 0, sizeof has);
    if (c.hasf) { molar(&M, &c.f, true); has[KD_MM] = true; v[KD_MM] = M; }
    for (i = 0; i < c.nq; i++) {
        const cq_t *q = &c.q[i];
        k = kind_of(&q->u);
        if (!k || is_count_unit(&q->u)) {           /* particles */
            if (has[KD_AMT]) { err = "ERR:UNITS"; return false; }
            calc(&v[KD_AMT], "n=N/NA=", &q->n, '/', &NA, "mol");
            k = KD_AMT;
            count = true;
        } else if (k == KD_OTHER || (has[k] && !(k == KD_MM && c.hasf))) {
            err = "ERR:UNITS";
            return false;
        } else {
            to_base(&v[k], q, k);
            if (v[k].v != q->n.v) {                 /* 250.0mL = 0.2500 L */
                char x[24], u[24], y[24];
                num_fmt_plain(q->n, x);
                unit_str(&q->u, u);
                num_fmt_plain(v[k], y);
                strcat(x, u);
                strcat(y, " ");
                work_add(W, x, "=", y, kname[k - 1]);
            }
        }
        has[k] = true;
    }
    M = v[KD_MM];                                   /* a typed g/mol wins */
    /* amount of substance */
    if (has[KD_AMT]) n = v[KD_AMT];
    else if (has[KD_MASS] && has[KD_MM]) calc(&n, "n=m/M=", &v[KD_MASS], '/', &M, "mol");
    else if (has[KD_CONC] && has[KD_VOL]) calc(&n, "n=MV=", &v[KD_CONC], '*', &v[KD_VOL], "mol");
    else hasn = false;
    if (has[KD_MASS] && !has[KD_MM]) { err = "ERR:NEED FORMULA"; return false; }

    if (c.hasel) {                      /* part of the compound */
        num_t Mw, ratio;
        fp_t rv = 0;
        char pe[24];
        for (i = 0; i < c.el.nel; i++) {
            fp_t x = (fp_t)formula_count(&c.f, c.el.el[i].z) / c.el.el[i].n;
            if (!i || x < rv) rv = x;
        }
        ratio = num_exact(rv);
        molar(&Mw, &c.el, false);
        formula_pretty(c.el.disp, pe);
        if (!hasn) {                    /* % by mass */
            num_t part, hund = num_exact(100);
            calc(&part, "part=", &ratio, '*', &Mw, NULL);
            calc(&r, "%=", &part, '/', &M, NULL);
            calc(&r, " ", &r, '*', &hund, "%");
            finish(&r, "%", pe, out, small);
            return true;
        }
        calc(&n, "n=", &ratio, '*', &n, "mol");
        if (!wu) {
            unit_parse(has[KD_MASS] ? "g" : "mol", &tmp);
            wu = &tmp;
        }
        wk = kind_of(wu);
        if (wk == KD_MASS) calc(&r, "m=nM=", &n, '*', &Mw, "g");
        else if (wk == KD_AMT) r = n;
        else { err = "ERR:UNITS"; return false; }
        answer(&r, wk, wu, pe, out, small);
        return true;
    }

    /* default "to" when none was typed */
    if (wu) wk = kind_of(wu);
    else if (!hasn && c.hasf && !c.nq) wk = KD_MM;
    else if (has[KD_VOL] && !has[KD_CONC] && hasn) wk = KD_CONC;
    else if (count) wk = KD_AMT;
    else if (has[KD_AMT]) {
        if (has[KD_MM]) wk = KD_MASS;
        else { unit_parse("particles", &tmp); wu = &tmp; wk = KD_AMT; }
    } else if (hasn) wk = KD_AMT;
    else { err = "ERR:SYNTAX"; return false; }

    err = "ERR:NEED AMOUNT";
    if (!hasn && wk != KD_MM) return false;
    switch (wk) {
    case KD_AMT:
        r = n;
        break;
    case KD_MASS:
        err = "ERR:NEED FORMULA";
        if (!has[KD_MM]) return false;
        calc(&r, "m=nM=", &n, '*', &M, "g");
        break;
    case KD_VOL:
        err = "ERR:NEED M";
        if (!has[KD_CONC]) return false;
        calc(&r, "V=n/M=", &n, '/', &v[KD_CONC], "L");
        break;
    case KD_CONC:
        err = "ERR:NEED VOLUME";
        if (!has[KD_VOL]) return false;
        calc(&r, "M=n/V=", &n, '/', &v[KD_VOL], "M");
        break;
    case KD_MM:
        err = "ERR:NEED FORMULA";
        if (!has[KD_MM]) return false;
        r = M;
        break;
    default:
        err = "ERR:UNITS";
        return false;
    }
    answer(&r, wk, wu, NULL, out, small);
    return true;
}

/* one quantity in, another unit out (no chemistry) */
static bool plain(char *out, char *small) {
    cq_t *q = &c.q[0];
    fp_t v;
    num_t r;
    char a[24], b[24], x[24];
    if (!c.haswant) {                   /* nothing to convert to: show it */
        unit_str(&q->u, a);
        finish(&q->n, a, NULL, out, small);
        return true;
    }
    if (!unit_convert(q->n.v, &q->u, &c.want, &v)) return false;
    r = q->num ? num_with_sf(v, q->n.sf) : num_exact(v);
    unit_str(&q->u, a);
    unit_str(&c.want, b);
    {
        udim_t d;
        unit_dim(&q->u, &d);
        if (!d.d[4]) {                  /* a factor line, not for temperature */
            unit_convert(1, &q->u, &c.want, &v);
            num_fmt_plain(num_exact(v), x);
            strcat(x, b);
            work_add(W, "1", a, "=", x);
        }
    }
    finish(&r, b, NULL, out, small);
    return true;
}

void convert_compute(const char *in, char *out, char *small, work_t *w) {
    uint8_t i, given = 0;
    bool dil = false;
    W = w;
    NA = num_exact(NA_VALUE);
    err = "ERR:SYNTAX";
    out[0] = small[0] = 0;
    if (!parse(in)) { strcpy(out, err); return; }
    for (i = 0; i < c.nq; i++) {
        if (c.q[i].side) dil = true;
        else given++;
    }
    if (!dil && !c.hasf && !c.hasel && given == 1 &&
        (c.haswant || kind_of(&c.q[0].u) == KD_OTHER)) {
        unit_t a = c.q[0].u, b = c.want;
        if (c.haswant) unit_resolve_pair(&c.q[0].u, &c.want);
        if (plain(out, small)) return;
        work_clear(w);
        c.q[0].u = a;               /* ".0450mol→m": try molar */
        c.want = b;
    }
    /* chemistry: every lone m is molar */
    for (i = 0; i < c.nq; i++) unit_resolve_m(&c.q[i].u, true);
    unit_resolve_m(&c.want, true);
    if (dil) {
        err = "ERR:NEED M AND V";
        if (dilution(out, small)) return;
    } else if (chem(out, small)) return;
    work_clear(w);
    small[0] = 0;
    strcpy(out, err);
}

/* [Y=] substances, [WINDOW] units: pick one, it is typed at the cursor */
void convert_field(uint8_t n) {
    const entry_t *e;
    uint8_t i;
    char txt[24];
    if (n > 1) return;
    i = n ? ui_pick("UNITS", unitlist, NUNITLIST) : ui_pick("SUBSTANCE", subst, NSUBST);
    if (i == 0xFF) return;
    e = app_entry();
    convert_pick_text(n ? unitlist[i] : subst[i], n, e->done || !e->cur ? 0 : e->buf[e->cur - 1], txt);
    app_insert(txt);
}
