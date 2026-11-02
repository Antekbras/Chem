/* info.c - INFO pages for one element, as in PTABLE v2:
 *   1 Element information  2 Orbital diagram  3 Electron configuration
 *   4 Lewis dot symbol
 * ◄ ► page, ▲ ▼ previous / next element, ENTER = use it on the ATOM line,
 * CLEAR = back. Text with '^' raises the digits / signs after it. */
#include <string.h>
#include "atom.h"
#include "plat.h"

#ifndef C_WHITE
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000
#endif
#define RED RGB(230, 0, 0)
#define DARK RGB(82, 80, 82)
#define ILH 18                       /* large-font line pitch */

static const char *const page_name[4] = {
    "Element information", "Orbital diagram", "Electron configuration", "Lewis dot symbol"};
static const char shell_ch[] = "KLMNOPQ";
static const char i_lch[] = "spdf";

static econfig_t ec;
static uint8_t Z, on, oe;           /* element, outer shell n and its electrons */
static char buf[128], b2[64];

/* ---- rich text: "1s^2" draws the 2 raised in the small font ---- */
static bool sup_ch(char c) { return (c >= '0' && c <= '9') || c == '+' || c == '-'; }

static uint16_t rich(const char *s, uint16_t x, uint8_t y, uint8_t f, uint16_t fg, uint16_t bg, bool draw) {
    char seg[48];
    while (*s) {
        bool sup = *s == '^';
        uint8_t n = 0, ff = sup ? (uint8_t)(f & UI_TRANS) : f;
        if (sup) s++;
        while (*s && n < sizeof seg - 1 && (sup ? sup_ch(*s) : *s != '^')) seg[n++] = *s++;
        seg[n] = 0;
        if (draw) ui_text(seg, x, (uint8_t)(sup ? y - 3 : y), ff, fg, bg);
        x = (uint16_t)(x + ui_text_width(seg, ff));
    }
    return x;
}

/* word-wrapped rich text from x0 to xmax; returns the y after it */
static uint8_t wrap(const char *s, uint16_t x0, uint8_t y, uint16_t xmax, uint8_t f, uint16_t bg) {
    uint16_t x = x0, sp = ui_text_width(" ", f);
    uint8_t lh = f & UI_LARGE ? ILH : 13;
    char w[40];
    while (*s) {
        uint8_t n = 0;
        uint16_t ww;
        while (*s == ' ') s++;
        while (*s && *s != ' ' && n < sizeof w - 1) w[n++] = *s++;
        w[n] = 0;
        if (!n) break;
        ww = rich(w, 0, 0, f, 0, 0, false);
        if (x > x0 && x + ww > xmax) { x = x0; y = (uint8_t)(y + lh); }
        x = (uint16_t)(rich(w, x, y, f, C_BLACK, bg, true) + sp);
    }
    return (uint8_t)(y + lh);
}

/* "1s2 2s2" -> "1s^2 2s^2"; "[Ar]4s2" -> "[Ar] 4s^2" */
static void cfg_rich(char *d, bool full) {
    char t[100], *s = t;
    econfig_str(&ec, full, t);
    for (; *s; s++) {
        *d++ = *s;
        if (*s == ']') *d++ = ' ';
        if ((*s == 's' || *s == 'p' || *s == 'd' || *s == 'f') && s[1] >= '0' && s[1] <= '9') *d++ = '^';
    }
    *d = 0;
}

static uint8_t shell_e(uint8_t n) {
    uint8_t i, e = 0;
    for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == n) e = (uint8_t)(e + ec.occ[i]);
    return e;
}

static char *put_shells(char *p) {
    uint8_t n;
    for (n = 1; n <= on; n++) p = fmt(p, n > 1 ? ", %u" : "%u", shell_e(n));
    return p;
}

/* subshells of shell n (l from lo to hi): "2s^2 2p^2" */
static char *put_subs(char *p, uint8_t n, uint8_t lo, uint8_t hi) {
    uint8_t i;
    for (i = 0; i < NUM_SUBSHELLS; i++)
        if (sub_n[i] == n && sub_l[i] >= lo && sub_l[i] <= hi && ec.occ[i]) {
            if (p > buf && p[-1] != '(' && p[-1] != ' ') *p++ = ' ';
            p = fmt(p, "%u%c^%u", n, i_lch[sub_l[i]], ec.occ[i]);
        }
    return p;
}

static bool radioactive(uint8_t z) { return z == 43 || z == 61 || z >= 84; }
static bool d_or_f(uint8_t z) { uint8_t g = el_group(z); return !g || (g >= 3 && g <= 12); }

/* ---- header: family color, symbol, name, page title, Z, page dots ---- */
static void header(uint8_t page) {
    uint8_t f = el_family(Z), i;
    uint16_t bg = fam_strong[f], fg = f == 6 || f == 7 || f == 9 ? C_WHITE : C_BLACK;
    char nm[16], z[4];
    ui_fill(0, 0, LCD_W, 34, bg);
    ui_fill(0, 34, LCD_W, 1, DARK);
    ui_text(elements[Z].sym, 6, 9, UI_LARGE | UI_BOLD | UI_TRANS, fg, bg);
    el_name(nm, Z);
    ui_text(nm, 50, 1, UI_LARGE | UI_BOLD | UI_TRANS, fg, bg);
    if (page == 1) {
        uint8_t u = unpaired(&ec);
        fmt(buf, "%s \x0C %u unpaired electron%s", page_name[1], u, u == 1 ? "" : "s");
    } else {
        strcpy(buf, page_name[page]);
    }
    ui_text(buf, 50, 20, UI_TRANS, fg, bg);
    put_u(z, Z);
    ui_text(z, (uint16_t)(314 - ui_text_width(z, UI_LARGE | UI_BOLD)), 1, UI_LARGE | UI_BOLD | UI_TRANS, fg, bg);
    for (i = 0; i < 4; i++) {
        uint16_t x = (uint16_t)(289 + 7 * i);
        ui_fill(x, 23, 5, 5, fg);
        if (i != page) ui_fill((uint16_t)(x + 1), 24, 3, 3, bg);
    }
    ui_fill(0, 35, LCD_W, LCD_H - 35, C_WHITE);
}

/* ---- page 1: element information ---- */
static uint8_t row(uint8_t i, const char *label) {
    uint8_t y = (uint8_t)(39 + 21 * i);
    uint16_t bg = i & 1 ? C_BAND : C_WHITE;
    ui_fill(0, y, LCD_W, 21, bg);
    ui_text(label, 12, (uint8_t)(y + 5), 0, C_GREY, bg);
    return y;
}

static void ival(uint8_t i, const char *s, uint8_t f, uint16_t fg) {
    ui_text(s, 122, (uint8_t)(39 + 21 * i + 2), (uint8_t)(UI_LARGE | f), fg, i & 1 ? C_BAND : C_WHITE);
}

static void radius(uint8_t i, const char *label, uint16_t r) {
    row(i, label);
    if (r) { fmt(buf, "%u pm", r); ival(i, buf, 0, C_BLACK); }
    else ival(i, "no data", 0, C_GREY);
}

static void page_info(void) {
    uint8_t g = el_group(Z), f = el_family(Z), y;
    char *p;
    row(0, "Atomic number");
    put_u(buf, Z);
    ival(0, buf, UI_BOLD | UI_TRANS, C_BLACK);
    row(1, "Atomic mass");
    if (mass_iupac[Z] % 1000 == 0) {         /* no standard weight: most stable isotope */
        fmt(buf, "[%u] u", Z + iso_n[Z]);
    } else {                                  /* IUPAC, "55.845", "6.94" */
        uint32_t m = mass_iupac[Z];
        char *p2 = fmt(buf, "%u.%c%c%c", (unsigned)(m / 1000), '0' + (int)(m / 100 % 10), '0' + (int)(m / 10 % 10),
                       '0' + (int)(m % 10));
        while (p2[-1] == '0') p2--;
        put_s(p2, " u");
    }
    ival(1, buf, 0, C_BLACK);
    row(2, "Group");
    if (g) put_u(buf, g); else strcpy(buf, "n/a (f-block series)");
    ival(2, buf, 0, C_BLACK);
    row(3, "Period");
    put_u(buf, el_period(Z));
    ival(3, buf, 0, C_BLACK);
    row(4, "Block");
    fmt(buf, "%c-block", el_block(Z));
    ival(4, buf, 0, C_BLACK);
    y = row(5, "Family");
    ui_fill(122, (uint8_t)(y + 6), 10, 10, fam_strong[f]);
    ui_text(family_name[f], 137, (uint8_t)(y + 2), UI_LARGE, C_BLACK, C_BAND);
    row(6, "Type");
    p = put_s(buf, type_name[elements[Z].cat]);
    buf[0] = (char)(buf[0] - 32);
    if (el_state(Z) == 3) put_s(p, " (predicted)");
    ival(6, buf, 0, C_BLACK);
    radius(7, "Atomic radius", rad_emp[Z]);
    radius(8, "Covalent radius", rad_cov[Z]);
}

/* ---- page 2: orbital diagram ---- */
static const uint16_t sub_col[4] = {RGB(205, 250, 205), RGB(156, 218, 238), RGB(255, 226, 148), RGB(246, 194, 238)};
static const uint8_t sub_x[4] = {40, 63, 112, 187};

static void arrow(uint16_t x, uint8_t y, bool up) {
    uint8_t t = up ? (uint8_t)(y + 2) : (uint8_t)(y + 11);
    int8_t d = up ? 1 : -1;
    ui_fill(x, (uint8_t)(y + 2), 1, 10, RED);
    ui_fill((uint16_t)(x - 1), (uint8_t)(t + d), 3, 1, RED);
    ui_fill((uint16_t)(x - 2), (uint8_t)(t + 2 * d), 5, 1, RED);
}

static void isotope_line(uint8_t y, uint16_t bg) {
    uint16_t x = 6;
    char nm[16];
    el_name(nm, Z);
    x = (uint16_t)(x + ui_text("Isotope: ", x, y, 0, C_GREY, bg));
    fmt(buf, "%s-%u", nm, Z + iso_n[Z]);
    x = (uint16_t)(x + ui_text(buf, x, y, UI_BOLD | UI_TRANS, C_BLACK, bg));
    ui_text(radioactive(Z) ? "  (most stable)" : "  (most common)", x, y, 0, C_GREY, bg);
    fmt(buf, "%u p^+   %u n^0", Z, iso_n[Z]);
    rich(buf, (uint16_t)(314 - rich(buf, 0, 0, 0, 0, 0, false)), y, 0, C_BLACK, bg, true);
}

static void page_orbitals(void) {
    uint8_t n, l, i, k;
    for (n = 1; n <= on; n++) {
        uint8_t y = (uint8_t)(40 + 26 * (n - 1));
        fmt(buf, "n=%u", n);
        ui_text(buf, 8, (uint8_t)(y + 1), UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
        fmt(buf, "%u e^-", shell_e(n));
        rich(buf, (uint16_t)(314 - rich(buf, 0, 0, 0, 0, 0, false)), (uint8_t)(y + 1), 0, C_GREY, C_WHITE, true);
        for (l = 0; l < 4; l++) {
            uint8_t o = (uint8_t)(2 * l + 1), e = 0;
            uint16_t x0 = sub_x[l];
            for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == n && sub_l[i] == l) e = ec.occ[i];
            if (!e) continue;
            for (k = 0; k < o; k++) {
                uint16_t bx = (uint16_t)(x0 + 13 * k);
                ui_fill(bx, y, 14, 14, C_BLACK);
                ui_fill((uint16_t)(bx + 1), (uint8_t)(y + 1), 12, 12, sub_col[l]);
                if (k < e) arrow((uint16_t)(bx + 4), y, true);       /* Hund: one up in each first */
                if (k + o < e) arrow((uint16_t)(bx + 9), y, false);
            }
            fmt(buf, "%u%c", n, i_lch[l]);
            ui_text(buf, (uint16_t)(x0 + (13 * o + 1) / 2 - ui_text_width(buf, 0) / 2), (uint8_t)(y + 14), 0,
                    C_BLACK, C_WHITE);
        }
    }
    ui_fill(0, 224, LCD_W, 1, RGB(213, 210, 222));
    ui_fill(0, 225, LCD_W, 15, C_BAND);
    isotope_line(226, C_BAND);
}

/* ---- page 3: electron configuration ---- */
static void page_config(void) {
    uint8_t y = 38;
    uint16_t x;
    char nm[16];
    ui_text("Expanded (full) configuration", 8, y, UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
    cfg_rich(buf, true);
    y = wrap(buf, 14, (uint8_t)(y + 15), 314, UI_LARGE, C_WHITE);
    ui_text("Noble gas notation", 8, (uint8_t)(y + 2), UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
    cfg_rich(buf, false);
    y = wrap(buf, 14, (uint8_t)(y + 17), 314, UI_LARGE, C_WHITE);
    ui_text("Electrons per shell", 8, (uint8_t)(y + 2), UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
    put_shells(buf);
    ui_text(buf, 14, (uint8_t)(y + 17), UI_LARGE, C_BLACK, C_WHITE);
    /* most common isotope, in nuclide notation */
    ui_fill(0, 170, LCD_W, 1, RGB(213, 210, 222));
    ui_fill(0, 171, LCD_W, 69, C_BAND);
    ui_text(radioactive(Z) ? "Most stable isotope" : "Most common isotope", 8, 174, UI_BOLD | UI_TRANS, DARK, C_BAND);
    put_u(b2, Z + iso_n[Z]);
    x = ui_text(b2, 10, 188, 0, C_BLACK, C_BAND);
    put_u(buf, Z);
    if (ui_text_width(buf, 0) > x) x = ui_text_width(buf, 0);
    ui_text(buf, 10, 207, 0, C_BLACK, C_BAND);
    ui_text(elements[Z].sym, (uint16_t)(11 + x), 196, UI_LARGE | UI_BOLD | UI_TRANS, C_BLACK, C_BAND);
    el_name(nm, Z);
    fmt(buf, "%s-%s", nm, b2);
    ui_text(buf, 68, 187, UI_LARGE | UI_BOLD | UI_TRANS, C_BLACK, C_BAND);
    fmt(buf, "Mass number %s,  atomic number %u", b2, Z);
    ui_text(buf, 68, 205, 0, C_GREY, C_BAND);
    fmt(buf, "%u protons \x0C %u neutrons \x0C %u electrons", Z, iso_n[Z], Z);
    ui_text(buf, 8, 225, 0, C_BLACK, C_BAND);
}

/* ---- page 4: Lewis dot symbol ---- */
/* the symbol at twice the large font, centered on (cx, cy) */
static void big(uint16_t cx, uint8_t cy, uint16_t *l, uint16_t *t, uint16_t *r, uint16_t *b) {
    uint16_t w = ui_text(elements[Z].sym, 4, 200, UI_LARGE | UI_BOLD, C_BLACK, C_BAND), i, j, *v = plat_vram();
    uint32_t rows[18];
    if (w > 32) w = 32;
    for (j = 0; j < 18; j++) {
        rows[j] = 0;
        for (i = 0; i < w; i++) if (v[(200 + j) * LCD_W + 4 + i] == C_BLACK) rows[j] |= 1UL << i;
    }
    ui_fill(4, 200, (uint16_t)(w + 1), 18, C_BAND);
    *l = (uint16_t)(cx - w);
    *t = (uint16_t)(cy - 18);
    *r = (uint16_t)(cx + w);
    *b = (uint16_t)(cy + 18);
    for (j = 0; j < 18; j++)
        for (i = 0; i < w; i++)
            if (rows[j] >> i & 1) ui_fill((uint16_t)(*l + 2 * i), (uint8_t)(*t + 2 * j), 2, 2, C_BLACK);
}

static void dot(uint16_t x, uint16_t y) { ui_fill((uint16_t)(x - 1), (uint8_t)(y - 1), 3, 3, RED); }

static uint8_t pair(uint8_t y, const char *label, const char *value) {
    rich(label, 138, y, 0, C_GREY, C_WHITE, true);
    return wrap(value, 138, (uint8_t)(y + 11), 318, UI_LARGE, C_WHITE);
}

static void page_lewis(void) {
    uint8_t side[4] = {0, 0, 0, 0}, i, s_e = 0, y = 38, u = unpaired(&ec);
    uint16_t l, t, r, b, cx = 66;
    bool df = d_or_f(Z);
    char *p;
    ui_fill(0, 35, 132, LCD_H - 35, C_BAND);
    for (i = 0; i < NUM_SUBSHELLS; i++) if (sub_n[i] == on && sub_l[i] == 0) s_e = ec.occ[i];
    if (df) oe = s_e;                       /* d / f: outer-shell s electrons only */
    big(cx, 104, &l, &t, &r, &b);
    if (Z == 2) side[1] = 2;
    else for (i = 0; i < oe && i < 8; i++) side[i & 3]++;
    for (i = 0; i < side[0]; i++) dot((uint16_t)(cx - (side[0] > 1 ? 4 : 0) + 8 * i), (uint16_t)(t - 5));
    for (i = 0; i < side[1]; i++) dot((uint16_t)(r + 5), (uint16_t)(104 - (side[1] > 1 ? 4 : 0) + 8 * i));
    for (i = 0; i < side[2]; i++) dot((uint16_t)(cx - (side[2] > 1 ? 4 : 0) + 8 * i), (uint16_t)(b + 4));
    for (i = 0; i < side[3]; i++) dot((uint16_t)(l - 6), (uint16_t)(104 - (side[3] > 1 ? 4 : 0) + 8 * i));
    ui_text("Lewis dot symbol", (uint16_t)(cx - ui_text_width("Lewis dot symbol", 0) / 2), 160, 0, C_GREY, C_BAND);
    fmt(buf, "%u dot%s", oe, oe == 1 ? "" : "s");
    ui_text(buf, (uint16_t)(cx - ui_text_width(buf, UI_BOLD) / 2), 174, UI_BOLD | UI_TRANS, C_BLACK, C_BAND);
    if (df) {
        strcpy(buf, "(outer-shell s e^- only)");
        rich(buf, (uint16_t)(cx - rich(buf, 0, 0, 0, 0, 0, false) / 2), 192, 0, C_GREY, C_BAND, true);
    }
    /* right column */
    p = fmt(buf, "%u  (", oe);
    p = put_subs(p, on, 0, df ? 0 : 3);
    put_s(p, ")");
    y = pair(y, df ? "Outer-shell (valence) e^-" : "Valence electrons", buf);
    fmt(buf, "n = %u  (%c shell)", on, shell_ch[on - 1]);
    y = pair(y, "Outermost shell", buf);
    if (df) {
        p = put_subs(buf, (uint8_t)(on - 2), 3, 3);
        *p = 0;
        p = put_subs(p, (uint8_t)(on - 1), 2, 2);
        y = pair(y, "Inner d / f electrons", buf[0] ? buf : "none");
    }
    put_shells(buf);
    y = pair(y, "Electrons per shell", buf);
    y = pair(y, df ? "Forming ions" : "Octet rule", ion_rule_text(Z, buf));
    fmt(buf, "%u  (%s)", u, u ? "paramagnetic" : "diamagnetic");
    pair(y, "Unpaired electrons", buf);
}

bool info_pages(uint8_t z) {
    uint8_t page = 0;
    for (;;) {
        uint8_t k;
        Z = z;
        econfig(z, 0, &ec);
        for (on = 0, k = 0; k < NUM_SUBSHELLS; k++) if (ec.occ[k] && sub_n[k] > on) on = sub_n[k];
        oe = shell_e(on);
        header(page);
        if (page == 0) page_info();
        else if (page == 1) page_orbitals();
        else if (page == 2) page_config();
        else page_lewis();
        k = wait_key();
        if (k == K_RIGHT) page = (uint8_t)((page + 1) & 3);
        else if (k == K_LEFT) page = (uint8_t)((page + 3) & 3);
        else if (k == K_DOWN) z = z < NUM_ELEMENTS ? (uint8_t)(z + 1) : 1;
        else if (k == K_UP) z = z > 1 ? (uint8_t)(z - 1) : NUM_ELEMENTS;
        else if (k == K_ENTER) { ast.tz = z; app_enter(elements[z].sym); return true; }
        else if (k == K_CLEAR || k == K_GRAPH || k == K_MODE || k == K_TRACE) { ast.tz = z; return false; }
    }
}
