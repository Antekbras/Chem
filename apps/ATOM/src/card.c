/* card.c - element facts and the element / ion / isotope card ([GRAPH]). */
#include <stdarg.h>
#include <string.h>
#include "atom.h"
#include "formula.h"

const char *const family_name[] = {
    "Alkali metal", "Alkaline earth metal", "Transition metal",
    "Post-transition metal", "Metalloid", "Nonmetal", "Halogen",
    "Noble gas", "Lanthanide", "Actinide",
};
const char *const state_name[] = {"solid", "liquid", "gas", "unknown"};
const char *const type_name[] = {"metal", "nonmetal", "metalloid", "nonmetal"};

char *put_u(char *p, uint16_t v) {
    char t[6];
    uint8_t n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}

char *put_s(char *p, const char *s) {
    while (*s) *p++ = *s++;
    *p = 0;
    return p;
}

static const uint8_t period_end[] = {2, 10, 18, 36, 54, 86, 118};

uint8_t el_period(uint8_t z) {
    uint8_t p = 0;
    while (z > period_end[p]) p++;
    return (uint8_t)(p + 1);
}

uint8_t el_group(uint8_t z) {
    uint8_t p = el_period(z), pos;
    if (z == 1) return 1;
    if (z == 2) return 18;
    pos = (uint8_t)(z - period_end[p - 2]);           /* 1-based in period */
    if (p <= 3) return pos <= 2 ? pos : (uint8_t)(pos + 10);
    if (p <= 5) return pos;
    if (pos <= 2) return pos;
    if (pos <= 17) return 0;                          /* La..Lu, Ac..Lr */
    return (uint8_t)(pos - 14);
}

char el_block(uint8_t z) {
    uint8_t g = el_group(z);
    if (!g) return 'f';
    if (g <= 2 || z == 2) return 's';
    if (g >= 13) return 'p';
    return 'd';
}

uint8_t el_family(uint8_t z) {
    uint8_t g = el_group(z);
    if (!g) return z < 89 ? 8 : 9;
    if (g == 18) return 7;
    if (g == 17) return 6;
    if (z == 1) return 5;
    if (g == 1) return 0;
    if (g == 2) return 1;
    if (g <= 12) return 2;
    if (elements[z].cat == CAT_METALLOID) return 4;
    if (elements[z].cat == CAT_METAL) return 3;
    return 5;
}

uint8_t el_state(uint8_t z) {
    return (uint8_t)((state_bits[z >> 2] >> (2 * (z & 3))) & 3);
}

static uint8_t orbitals(uint8_t i) { return (uint8_t)(2 * sub_l[i] + 1); }

uint8_t unpaired(const econfig_t *c) {
    uint8_t i, u = 0;
    for (i = 0; i < NUM_SUBSHELLS; i++) {
        uint8_t o = orbitals(i), e = c->occ[i];
        u = (uint8_t)(u + (e <= o ? e : 2 * o - e));
    }
    return u;
}

/* outermost shell n and its electrons */
static uint8_t outer(const econfig_t *c, uint8_t *n) {
    uint8_t i, e = 0;
    *n = 0;
    for (i = 0; i < NUM_SUBSHELLS; i++) if (c->occ[i] && sub_n[i] > *n) *n = sub_n[i];
    for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == *n) e = (uint8_t)(e + c->occ[i]);
    return e;
}

static char *put_sym(char *p, uint8_t z) { return put_s(p, elements[z].sym); }

static char *put_charge(char *p, int8_t q) {
    if (!q) return p;
    if (q > 1 || q < -1) p = put_u(p, (uint16_t)(q < 0 ? -q : q));
    *p++ = q > 0 ? '+' : '-';
    *p = 0;
    return p;
}

/* tiny printf: %u unsigned, %s string, %c char, %S symbol of Z, %q charge */
static char *vfmt(char *p, const char *f, va_list ap) {
    for (; *f; f++) {
        if (*f != '%') { *p++ = *f; continue; }
        switch (*++f) {
        case 'u': p = put_u(p, (uint16_t)va_arg(ap, unsigned)); break;
        case 's': p = put_s(p, va_arg(ap, const char *)); break;
        case 'c': *p++ = (char)va_arg(ap, int); break;
        case 'S': p = put_sym(p, (uint8_t)va_arg(ap, int)); break;
        case 'q': p = put_charge(p, (int8_t)va_arg(ap, int)); break;
        }
    }
    *p = 0;
    return p;
}

char *fmt(char *p, const char *f, ...) {
    va_list ap;
    va_start(ap, f);
    p = vfmt(p, f, ap);
    va_end(ap);
    return p;
}

void wf(work_t *w, const char *f, ...) {
    char line[64];
    va_list ap;
    va_start(ap, f);
    vfmt(line, f, ap);
    va_end(ap);
    work_add(w, line, NULL, NULL, NULL);
}

/* "55.845" from mg/mol; radioactive: "[98]" */
char *put_mass(char *p, uint8_t z) {
    uint32_t m = elements[z].mass1000;
    char d[4];
    int8_t k;
    bool radio = z == 43 || z == 61 || z >= 84;
    if (radio && m % 1000 == 0) *p++ = '[';
    p = put_u(p, (uint16_t)(m / 1000));
    if (radio && m % 1000 == 0) return put_s(p, "]");
    m %= 1000;
    for (k = 2; k >= 0; k--) { d[k] = (char)('0' + m % 10); m /= 10; }
    for (k = 2; k > 0 && d[k] == '0'; k--) ;
    d[k + 1] = 0;
    *p++ = '.';
    return put_s(p, d);
}

/* a config too long for one line wraps at a space */
static void add_wrapped(work_t *w, const char *s) {
    while (strlen(s) > WORK_COLS) {
        char line[WORK_COLS + 1];
        uint8_t cut = WORK_COLS;
        while (cut && s[cut] != ' ') cut--;
        if (!cut) cut = WORK_COLS;
        memcpy(line, s, cut);
        line[cut] = 0;
        work_add(w, line, NULL, NULL, NULL);
        s += cut + (s[cut] == ' ');
    }
    work_add(w, s, NULL, NULL, NULL);
}

/* orbital box diagram of the valence subshells: "3d|↑↓|↑ |↑ |↑ |↑ |" */
static void boxes(const econfig_t *c, work_t *w) {
    static const char lch[] = "spdf";
    uint8_t i, n, top;
    outer(c, &top);
    for (i = c->ncore; i < NUM_SUBSHELLS; i++) {
        char line[WORK_COLS + 1], *p = line;
        uint8_t o, e, k;
        e = c->occ[i];
        if (!e || (e == 4 * sub_l[i] + 2 && sub_n[i] != top)) continue;
        o = orbitals(i);
        n = sub_n[i];
        *p++ = (char)('0' + n);
        *p++ = lch[sub_l[i]];
        *p++ = '|';
        for (k = 0; k < o; k++) {           /* Hund: one up in each, then pair */
            *p++ = k < e ? '\x1E' : ' ';
            *p++ = k + o < e ? '\x1F' : ' ';
            *p++ = '|';
        }
        *p = 0;
        work_add(w, line, NULL, NULL, NULL);
    }
}

/* Lewis dot symbol, 3 lines: dots top, right, bottom, left, then pairs */
static void lewis(uint8_t z, uint8_t dots, work_t *w) {
    uint8_t side[4] = {0, 0, 0, 0}, i;
    char l1[12], l2[WORK_COLS + 1], l3[12], *p;
    if (dots == 2 && z == 2) side[0] = 2;
    else for (i = 0; i < dots && i < 8; i++) side[i & 3]++;
    memset(l1, ' ', 6); l1[6] = 0;
    strcpy(l3, l1);
    if (side[0]) { l1[6] = '\x0C'; l1[7] = side[0] > 1 ? '\x0C' : 0; l1[8] = 0; }
    if (side[2]) { l3[6] = '\x0C'; l3[7] = side[2] > 1 ? '\x0C' : 0; l3[8] = 0; }
    p = put_s(l2, "Lewis");
    *p++ = side[3] > 1 ? ':' : side[3] ? '\x0C' : ' ';
    p = put_sym(p, z);
    if (side[1]) *p++ = side[1] > 1 ? ':' : '\x0C';
    *p = 0;
    if (side[0]) work_add(w, l1, NULL, NULL, NULL);
    work_add(w, l2, NULL, NULL, NULL);
    if (side[2]) work_add(w, l3, NULL, NULL, NULL);
}

/* how the element reaches a noble-gas configuration */
static void ion_rule(uint8_t z, uint8_t g, work_t *w) {
    uint8_t cat = elements[z].cat;
    const char *s = NULL;
    if (z == 1) s = "Shares 1e- (or H+ / H-)";
    else if (z == 2) s = "Full shell (duet): stable";
    else if (g == 18) s = "Full octet: very stable";
    else if (g == 0 || (g >= 3 && g <= 12)) s = "Loses outer s e- first";
    else if (g == 14 && cat == CAT_METAL) s = "Loses 2 or 4 e- -> cation";
    if (s) work_add(w, s, NULL, NULL, NULL);
    else if (cat == CAT_METAL) {
        uint8_t k = g > 12 ? (uint8_t)(g - 10) : g;
        wf(w, "Loses %ue- -> %u+ cation", k, k);
    } else if (g >= 15 && cat == CAT_NONMETAL) {
        wf(w, "Gains %ue- -> %u- anion", 18 - g, 18 - g);
    } else {
        wf(w, "Shares %ue- (covalent)", g > 14 ? 18 - g : g - 10);
    }
}

void card(uint8_t z, int8_t charge, work_t *w) {
    char full[120], *p;
    econfig_t c;
    uint8_t g = el_group(z), on, oe, u, d, f = 0, i, n;
    econfig(z, charge, &c);
    if (charge) {
        wf(w, "%S%q %u e-", z, charge, c.electrons);
    } else {
        uint16_t en = elements[z].en100;
        wf(w, "%u %S %s", z, z, elements[z].name);
        p = put_mass(put_s(full, "Mass "), z);
        if (en) fmt(p, " EN %u.%c%c", en / 100, '0' + en / 10 % 10, '0' + en % 10);
        work_add(w, full, NULL, NULL, NULL);
        p = full;
        if (g) p = fmt(p, "Group %u ", g);
        fmt(p, "Period %u %c-block", el_period(z), el_block(z));
        work_add(w, full, NULL, NULL, NULL);
        work_add(w, family_name[el_family(z)], NULL, NULL, NULL);
        wf(w, "%s, %s at 25\x14" "C", type_name[elements[z].cat], state_name[el_state(z)]);
        p = full;
        *p = 0;
        if (rad_emp[z]) p = fmt(p, "r %upm ", rad_emp[z]);
        if (rad_cov[z]) fmt(p, "cov.r %upm", rad_cov[z]);
        if (full[0]) work_add(w, full, NULL, NULL, NULL);
        wf(w, "Most %s: %S-%u", z == 43 || z == 61 || z >= 84 ? "stable" : "common", z, z + iso_n[z]);
        wf(w, "  %up %un %ue", z, iso_n[z], z);
    }
    econfig_str(&c, true, full);
    add_wrapped(w, full);
    if (c.core) {
        econfig_str(&c, false, full);
        add_wrapped(w, full);
    }
    /* electrons per shell, K L M N O P Q */
    p = put_s(full, "Shells");
    for (n = 1; n <= 7; n++) {
        uint8_t s = 0;
        for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == n) s = (uint8_t)(s + c.occ[i]);
        if (!s) break;
        p = fmt(p, " %u", s);
    }
    work_add(w, full, NULL, NULL, NULL);
    oe = outer(&c, &on);
    d = econfig_dcount(&c);
    for (i = c.ncore; i < NUM_SUBSHELLS; i++) if (sub_l[i] == 3) f = (uint8_t)(f + c.occ[i]);
    if (charge) {
        if (d) wf(w, "d electrons: %u", d);
    } else {
        p = fmt(full, "Valence e- %u (n=%u)", oe, on);
        if (d) p = fmt(p, " d%u", d);
        if (f) fmt(p, " f%u", f);
        work_add(w, full, NULL, NULL, NULL);
    }
    u = unpaired(&c);
    wf(w, "%u unpaired: %s", u, u ? "paramagnetic" : "diamagnetic");
    boxes(&c, w);
    if (!charge) {
        lewis(z, oe, w);
        p = put_s(full, "Ions");
        for (i = 0; i < 3 && elements[z].ions[i]; i++) p = fmt(p, " %S%q", z, elements[z].ions[i]);
        if (i) work_add(w, full, NULL, NULL, NULL);
        ion_rule(z, g, w);
    }
}

void isotope_card(uint8_t z, uint16_t a, int8_t charge, char *out, char *small, work_t *w) {
    uint16_t n = (uint16_t)(a - z);
    uint8_t e = (uint8_t)(z - charge);
    fmt(out, "%up %un %ue", z, n, e);
    fmt(small, "%S-%u%q", z, a, charge);
    wf(w, "%s %s", small, elements[z].name);
    wf(w, "mass number A = %u", a);
    wf(w, "protons = Z = %u", z);
    wf(w, "neutrons = A-Z = %u", n);
    if (charge) wf(w, "electrons = Z%c%u = %u", charge > 0 ? '-' : '+', charge > 0 ? charge : -charge, e);
    else wf(w, "electrons = Z = %u", e);
}
