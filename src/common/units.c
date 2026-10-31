/* units.c - unit parser and converter */
#include <string.h>
#include "units.h"

#define NA_VALUE 6.022e23

typedef struct {
    const char *key;    /* lowercase typed form */
    const char *disp;   /* display form */
    int8_t d[NDIM];
    fp_t f;             /* to SI */
    fp_t off;           /* temperature offset to kelvin */
    uint8_t prefix;     /* accepts metric prefixes */
} udef_t;

/* order matters only for the U_* enum at the top */
static const udef_t units[] = {
    {"m",     "m",    {1, 0, 0, 0, 0}, 1, 0, 1},
    {"g",     "g",    {0, 1, 0, 0, 0}, 1e-3, 0, 1},
    {"s",     "s",    {0, 0, 1, 0, 0}, 1, 0, 1},
    {"l",     "L",    {3, 0, 0, 0, 0}, 1e-3, 0, 1},
    {"mol",   "mol",  {0, 0, 0, 1, 0}, 1, 0, 1},
    {"M",     "M",    {-3, 0, 0, 1, 0}, 1e3, 0, 1},
    {"particles", "particles", {0, 0, 0, 1, 0}, 1 / NA_VALUE, 0, 0},
    {"atoms", "atoms", {0, 0, 0, 1, 0}, 1 / NA_VALUE, 0, 0},
    {"molecules", "molecules", {0, 0, 0, 1, 0}, 1 / NA_VALUE, 0, 0},
    {"ions",  "ions", {0, 0, 0, 1, 0}, 1 / NA_VALUE, 0, 0},
    {"j",     "J",    {2, 1, -2, 0, 0}, 1, 0, 1},
    {"cal",   "cal",  {2, 1, -2, 0, 0}, 4.184, 0, 1},
    {"ev",    "eV",   {2, 1, -2, 0, 0}, 1.602176634e-19, 0, 1},
    {"hz",    "Hz",   {0, 0, -1, 0, 0}, 1, 0, 1},
    {"pa",    "Pa",   {-1, 1, -2, 0, 0}, 1, 0, 1},
    {"atm",   "atm",  {-1, 1, -2, 0, 0}, 101325, 0, 0},
    {"torr",  "torr", {-1, 1, -2, 0, 0}, 133.322368, 0, 0},
    {"mmhg",  "mmHg", {-1, 1, -2, 0, 0}, 133.322368, 0, 0},
    {"k",     "K",    {0, 0, 0, 0, 1}, 1, 0, 0},
    {"c",     "\x14" "C", {0, 0, 0, 0, 1}, 1, 273.15, 0},
    {"f",     "\x14" "F", {0, 0, 0, 0, 1}, 5.0 / 9, 255.3722222222222, 0},
    {"in",    "in",   {1, 0, 0, 0, 0}, 0.0254, 0, 0},
    {"ft",    "ft",   {1, 0, 0, 0, 0}, 0.3048, 0, 0},
    {"yd",    "yd",   {1, 0, 0, 0, 0}, 0.9144, 0, 0},
    {"mi",    "mi",   {1, 0, 0, 0, 0}, 1609.344, 0, 0},
    {"min",   "min",  {0, 0, 1, 0, 0}, 60, 0, 0},
    {"hr",    "hr",   {0, 0, 1, 0, 0}, 3600, 0, 0},
    {"h",     "hr",   {0, 0, 1, 0, 0}, 3600, 0, 0},
    {"day",   "day",  {0, 0, 1, 0, 0}, 86400, 0, 0},
    {"lb",    "lb",   {0, 1, 0, 0, 0}, 0.45359237, 0, 0},
    {"oz",    "oz",   {0, 1, 0, 0, 0}, 0.028349523125, 0, 0},
    {"gal",   "gal",  {3, 0, 0, 0, 0}, 3.785411784e-3, 0, 0},
    {"qt",    "qt",   {3, 0, 0, 0, 0}, 9.46352946e-4, 0, 0},
    {"cc",    "cc",   {3, 0, 0, 0, 0}, 1e-6, 0, 0},
    {"amu",   "amu",  {0, 1, 0, 0, 0}, 1.66053907e-27, 0, 0},
};
#define NUNITS (sizeof units / sizeof units[0])

typedef struct { char c; int8_t e; } prefix_t;
static const prefix_t prefixes[] = {
    {'p', -12}, {'n', -9}, {'u', -6}, {'m', -3}, {'c', -2}, {'d', -1},
    {'k', 3}, {'M', 6}, {'G', 9}, {'T', 12},
};
#define NPREFIX (sizeof prefixes / sizeof prefixes[0])

static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static bool isletter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool isnum(char c) { return c >= '0' && c <= '9'; }

/* length of key matched at s (case-insensitive except molar "M"), 0 if none */
static uint8_t match(const char *s, const char *key) {
    uint8_t i;
    if (key[0] == 'M' && key[1] == 0) return 0;   /* molar only via lone m / "M" */
    for (i = 0; key[i]; i++) if (lower(s[i]) != key[i]) return 0;
    return i;
}

#define U_HZ 13  /* index of "hz" in units[] */

/* one unit word at s: longest match; plain units beat prefix + unit on ties */
static uint8_t parse_word(const char *s, uterm_t *t, bool *ambig) {
    uint8_t best = 0, i, k, len;
    char c0 = lower(s[0]);
    *ambig = false;
    for (i = 0; i < NUNITS; i++) {
        len = match(s, units[i].key);
        if (len > best) { best = len; t->id = i; t->pre = 0; }
    }
    for (k = 0; k < NPREFIX; k++) {
        int8_t e = prefixes[k].e;
        if (prefixes[k].c != c0) continue;     /* M G T are never typed */
        for (i = 0; i < NUNITS; i++) {
            if (!units[i].prefix) continue;
            len = match(s + 1, units[i].key);
            if (!len || len + 1 <= best) continue;
            best = (uint8_t)(len + 1);
            t->id = i;
            /* "mhz" means MHz: nobody means millihertz */
            t->pre = (i == U_HZ && c0 == 'm') ? 6 : e;
        }
    }
    if (best < 3 && (c0 == 'g' || c0 == 't') && match(s + 1, "hz")) {
        best = 3; t->id = U_HZ; t->pre = c0 == 'g' ? 9 : 12;
    }
    if (best && isletter(s[best])) {
        /* plurals: "hrs", "mols", "moles"; anything else is not a unit */
        char n1 = lower(s[best]), n2 = lower(s[best + 1]);
        if (n1 == 's' && !isletter(s[best + 1]) && t->id != U_SECOND) best++;
        else if (t->id == U_MOL && n1 == 'e' && n2 == 's' && !isletter(s[best + 2])) best += 2;
        else if (t->id == U_MOL && n1 == 'e' && !isletter(s[best + 1])) best++;
        else return 0;
    }
    if (best) {
        t->pow = 1;
        if (s[0] == 'M' && best == 1) { t->id = U_MOLAR; t->pre = 0; }
        else if (t->id == U_METER && t->pre == 0 && best == 1) *ambig = true;
    }
    return best;
}

uint8_t unit_parse(const char *s, unit_t *u) {
    const char *p = s;
    int8_t sign = 1;
    memset(u, 0, sizeof *u);
    for (;;) {
        uterm_t t;
        bool amb;
        uint8_t len = parse_word(p, &t, &amb);
        const char *q;
        if (!len || u->n >= UNIT_MAXT) break;
        q = p + len;
        if (*q == '^') q++;
        if (*q == CH_SQ) { t.pow = 2; q++; }
        else if (*q == CH_CUBE) { t.pow = 3; q++; }
        else if (isnum(*q) && *q != '0' && !isnum(q[1])) { t.pow = (int8_t)(*q - '0'); q++; }
        else if (q[-1] == '^') q--;
        if (t.pow != 1) amb = false;           /* m2 is area, never molar */
        t.pow = (int8_t)(t.pow * sign);
        if (amb) u->ambig |= (uint8_t)(1 << u->n);
        u->t[u->n++] = t;
        p = q;
        if ((*p == '/' || *p == '*' || *p == CH_DOT) && isletter(p[1])) {
            sign = *p == '/' ? -1 : 1;
            p++;
        } else break;
    }
    if (!u->n) return 0;
    return (uint8_t)(p - s);
}

bool unit_is_empty(const unit_t *u) { return u->n == 0; }

void unit_dim(const unit_t *u, udim_t *o) {
    uint8_t i, k;
    memset(o, 0, sizeof *o);
    o->f = 1;
    for (i = 0; i < u->n; i++) {
        const udef_t *d = &units[u->t[i].id];
        fp_t f = d->f * fp_pow10(u->t[i].pre);
        int8_t pw = u->t[i].pow;
        for (k = 0; k < NDIM; k++) o->d[k] = (int8_t)(o->d[k] + d->d[k] * pw);
        o->f *= FP_POW(f, pw);
    }
}

bool unit_same_dim(const udim_t *a, const udim_t *b) {
    return !memcmp(a->d, b->d, NDIM);
}

static bool is_temp(const unit_t *u) {
    return u->n == 1 && u->t[0].pow == 1 && units[u->t[0].id].d[4] == 1;
}

bool unit_convert(fp_t v, const unit_t *from, const unit_t *to, fp_t *out) {
    udim_t a, b;
    unit_dim(from, &a);
    unit_dim(to, &b);
    if (!unit_same_dim(&a, &b)) return false;
    if (is_temp(from) && is_temp(to)) {
        fp_t kel = v * a.f + units[from->t[0].id].off;
        *out = (kel - units[to->t[0].id].off) / b.f;
    } else {
        *out = v * a.f / b.f;
    }
    return true;
}

void unit_resolve_m(unit_t *u, bool molar) {
    uint8_t i;
    for (i = 0; i < u->n; i++)
        if (u->ambig & (1 << i)) u->t[i].id = molar ? U_MOLAR : U_METER;
    u->ambig = 0;
}

static uint8_t nbits(uint8_t m) { uint8_t c = 0; while (m) { c += m & 1; m >>= 1; } return c; }

/* set ambiguous terms of u from the bits of `choice` (bit set = molar) */
static void apply(unit_t *u, uint8_t choice) {
    uint8_t i, k = 0;
    for (i = 0; i < u->n; i++)
        if (u->ambig & (1 << i)) u->t[i].id = (choice >> k++) & 1 ? U_MOLAR : U_METER;
}

bool unit_resolve_pair(unit_t *from, unit_t *to) {
    uint8_t na = nbits(from->ambig), nb = nbits(to->ambig), all = (uint8_t)(na + nb), c;
    /* fewest molar choices first, so meter wins when both work */
    uint8_t want;
    for (want = 0; want <= all; want++) {
        for (c = 0; c < (1 << all); c++) {
            udim_t a, b;
            if (nbits(c) != want) continue;
            apply(from, c);
            apply(to, (uint8_t)(c >> na));
            unit_dim(from, &a);
            unit_dim(to, &b);
            if (unit_same_dim(&a, &b)) { from->ambig = 0; to->ambig = 0; return true; }
        }
    }
    unit_resolve_m(from, false);
    unit_resolve_m(to, false);
    return false;
}

void unit_mul(unit_t *a, const unit_t *b, int8_t sign) {
    uint8_t i, k;
    for (i = 0; i < b->n; i++) {
        uterm_t t = b->t[i];
        t.pow = (int8_t)(t.pow * sign);
        for (k = 0; k < a->n; k++)
            if (a->t[k].id == t.id && a->t[k].pre == t.pre) break;
        if (k < a->n) {
            a->t[k].pow = (int8_t)(a->t[k].pow + t.pow);
            if (!a->t[k].pow) {           /* cancelled: remove term */
                for (; k + 1 < a->n; k++) a->t[k] = a->t[k + 1];
                a->n--;
            }
        } else if (a->n < UNIT_MAXT) {
            if (b->ambig & (1 << i)) a->ambig |= (uint8_t)(1 << a->n);
            a->t[a->n++] = t;
        }
    }
}

static char *put_term(char *p, const uterm_t *t, int8_t pw) {
    const char *d = units[t->id].disp;
    if (t->pre) {
        uint8_t k;
        for (k = 0; k < NPREFIX; k++) if (prefixes[k].e == t->pre) break;
        *p++ = k < NPREFIX ? (prefixes[k].c == 'u' ? 'u' : prefixes[k].c) : '?';
    }
    while (*d) *p++ = *d++;
    if (pw == 2) *p++ = CH_SQ;
    else if (pw == 3) *p++ = CH_CUBE;
    else if (pw > 3) { *p++ = '^'; *p++ = (char)('0' + pw); }
    return p;
}

uint8_t unit_str(const unit_t *u, char *buf) {
    char *p = buf;
    uint8_t i;
    bool any = false;
    for (i = 0; i < u->n; i++) {
        if (u->t[i].pow <= 0) continue;
        if (any) *p++ = CH_DOT;
        p = put_term(p, &u->t[i], u->t[i].pow);
        any = true;
    }
    for (i = 0; i < u->n; i++) {
        if (u->t[i].pow >= 0) continue;
        if (!any) { *p++ = '1'; any = true; }
        *p++ = '/';
        p = put_term(p, &u->t[i], (int8_t)-u->t[i].pow);
    }
    *p = 0;
    return (uint8_t)(p - buf);
}

uint8_t qty_parse(const char *s, qty_t *q) {
    uint8_t n = num_parse(s, &q->n), k;
    memset(&q->u, 0, sizeof q->u);
    if (!n) return 0;
    k = unit_parse(s + n, &q->u);
    return (uint8_t)(n + k);
}
