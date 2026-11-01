/* orbital.c - orbital reference ([ZOOM]), 10 pages:
 * 1 quantum numbers  2 number table  3 Aufbau tree  4 filling order
 * 5 capacity  6-9 s p d f shapes  0 Hund's rule.
 * ◄ ► (or ENTER) previous / next, 1-9 0 jump, CLEAR back. */
#include <string.h>
#include "atom.h"
#include "plat.h"

#define RGB(r, g, b) (uint16_t)((((b) >> 3) << 11) | (((g) >> 2) << 5) | ((r) >> 3))
#define BAR 0xD304
#define LH 14
#define NL 14

static char pg[NL][48];
static uint8_t nl;

static const char *const titles[10] = {
    "Quantum numbers", "Number table", "Aufbau tree", "Filling order", "Capacity",
    "s orbital", "p orbitals", "d orbitals", "f orbitals", "Hund's rule"};
static const char lch[] = "spdf";

static char *ln(void) { return nl < NL ? pg[nl++] : pg[NL - 1]; }
static void add(const char *s) { strcpy(ln(), s); }

/* "|↑↓|↑ |  |" for e electrons in o orbitals */
static char *obox(char *p, uint8_t o, uint8_t e) {
    uint8_t k;
    *p++ = '|';
    for (k = 0; k < o; k++) {
        *p++ = k < e ? '\x1E' : ' ';
        *p++ = k + o < e ? '\x1F' : ' ';
        *p++ = '|';
    }
    *p = 0;
    return p;
}

static char *put_ml(char *p, uint8_t l) {
    if (!l) return put_s(p, "0");
    *p++ = '-';
    p = put_u(p, l);
    p = put_s(p, " to +");
    return put_u(p, l);
}

static void page(uint8_t t) {
    uint8_t n, l, i;
    char *p;
    nl = 0;
    switch (t) {
    case 0:
        add("n   energy level: 1, 2, 3 ...");
        add("l   sublevel: 0 to n-1");
        add("ml  orbital: -l to +l");
        add("ms  spin: +1/2 (up) or -1/2 (down)");
        add("l = 0 1 2 3  is  s p d f");
        add("Each orbital holds 2 e-, opposite spins.");
        add("Pauli: no two e- have all 4 the same.");
        add("Example 3p: n=3, l=1, ml=-1,0,+1");
        break;
    case 1:
        add("sub   l   ml           orbitals  e-");
        for (n = 1; n <= 4; n++)
            for (l = 0; l < n; l++) {
                p = ln();
                *p++ = (char)('0' + n);
                *p++ = lch[l];
                p = put_s(p, "    ");
                p = put_u(p, l);
                p = put_s(p, "   ");
                p = put_ml(p, l);
                while (p < pg[nl - 1] + 25) *p++ = ' ';
                p = put_u(p, (uint16_t)(2 * l + 1));
                p = put_s(p, "        ");
                p = put_u(p, (uint16_t)(4 * l + 2));
            }
        add("ms = +1/2 or -1/2 for every orbital");
        break;
    case 2:
        for (n = 1; n <= 7; n++) {
            p = ln();
            for (l = 0; l < n && l < 4 && n + l <= 8; l++) {
                if (l) *p++ = ' ';
                *p++ = (char)('0' + n);
                *p++ = lch[l];
            }
            *p = 0;
        }
        add("Follow the diagonals \x05 top to bottom:");
        add("1s 2s 2p 3s 3p 4s 3d 4p 5s 4d 5p");
        add("6s 4f 5d 6p 7s 5f 6d 7p");
        break;
    case 3:
        add("sub max sum end    sub max sum end");
        for (i = 0; i < 10; i++) {
            uint8_t j;
            p = ln();
            for (j = i; j < NUM_SUBSHELLS; j = (uint8_t)(j + 10)) {
                uint8_t cap = (uint8_t)(4 * sub_l[j] + 2), s = 0, k;
                char *q = p;
                for (k = 0; k <= j; k++) s = (uint8_t)(s + 4 * sub_l[k] + 2);
                *p++ = (char)('0' + sub_n[j]);
                *p++ = lch[sub_l[j]];
                *p++ = ' ';
                p = put_u(p, cap);
                while (p < q + 7) *p++ = ' ';
                p = put_u(p, s);
                while (p < q + 11) *p++ = ' ';
                p = put_s(p, elements[s].sym);
                while (p < q + 19) *p++ = ' ';
                *p = 0;
            }
        }
        add("sum = total so far, end = element filling it");
        break;
    case 4:
        add("s: 1 orbital   2 e-     p: 3 orbitals   6 e-");
        add("d: 5 orbitals 10 e-     f: 7 orbitals 14 e-");
        add("Each orbital holds 2 e-.");
        add("Shell n: n^2 orbitals, max 2n^2 e-");
        for (n = 1; n <= 4; n++) {
            p = put_s(ln(), "n=");
            p = put_u(p, n);
            p = put_s(p, "  sublevels ");
            for (l = 0; l < n; l++) *p++ = lch[l];
            p = put_s(p, "  orbitals ");
            p = put_u(p, (uint16_t)(n * n));
            p = put_s(p, "  max e- ");
            put_u(p, (uint16_t)(2 * n * n));
        }
        break;
    case 5:
        add("Shape: sphere");
        add("1 orbital, ml = 0");
        add("In every level n = 1, 2, 3 ...");
        add("Holds 2 e-");
        obox(put_s(ln(), "ns "), 1, 2);
        break;
    case 6:
        add("Shape: dumbbell (2 lobes)");
        add("3 orbitals px py pz");
        add("ml = -1, 0, +1");
        add("In levels n >= 2");
        add("Holds 6 e-");
        obox(put_s(ln(), "np "), 3, 6);
        break;
    case 7:
        add("Shape: cloverleaf (4 lobes)");
        add("dz^2: dumbbell + ring");
        add("5 orbitals: dxy dxz dyz");
        add("    dx^2-y^2 dz^2");
        add("ml = -2 to +2");
        add("In levels n >= 3, holds 10 e-");
        obox(put_s(ln(), "nd "), 5, 10);
        break;
    case 8:
        add("Shape: multi-lobed");
        add("(2 of 7 shown: fz^3, fx(x^2-3y^2))");
        add("7 orbitals, ml = -3 to +3");
        add("In levels n >= 4, holds 14 e-");
        obox(put_s(ln(), "nf "), 7, 14);
        break;
    case 9:
        add("Fill every box with one \x1E first,");
        add("then pair up with a \x1F.");
        for (i = 1; i <= 6; i++) {
            p = put_s(ln(), "p");
            *p++ = (char)('0' + i);
            *p++ = ' ';
            obox(p, 3, i);
        }
        add("Shown for p. The same rule applies to d and f.");
        break;
    }
}

static void disc(int16_t cx, int16_t cy, int16_t r, uint16_t col) {
    int16_t y, x;
    for (y = (int16_t)-r; y <= r; y++)
        for (x = (int16_t)-r; x <= r; x++)
            if (x * x + y * y <= r * r) px_fill((uint16_t)(cx + x), (uint16_t)(cy + y), 1, 1, col);
}

/* lobes on a circle of radius d around (cx,cy) */
static void lobes(uint8_t n, int16_t d, int16_t r) {
    /* unit directions for 2, 4, 6 lobes (x1000) */
    static const int16_t dx[6] = {0, 0, 707, -707, 707, -707}, dy[6] = {-1000, 1000, -707, 707, 707, -707};
    static const int16_t fx[6] = {1000, 500, -500, -1000, -500, 500}, fy[6] = {0, 866, 866, 0, -866, -866};
    uint8_t i;
    for (i = 0; i < n; i++) {
        int16_t ux = n == 6 ? fx[i] : n == 4 ? dx[i + 2] : dx[i];
        int16_t uy = n == 6 ? fy[i] : n == 4 ? dy[i + 2] : dy[i];
        disc((int16_t)(250 + (int32_t)ux * d / 1000), (int16_t)(150 + (int32_t)uy * d / 1000), r,
             (i & 1) ? RGB(120, 160, 255) : RGB(255, 130, 130));
    }
}

static void shape(uint8_t t) {
    if (t == 5) disc(250, 150, 34, RGB(120, 160, 255));
    else if (t == 6) lobes(2, 24, 20);
    else if (t == 7) lobes(4, 26, 16);
    else if (t == 8) lobes(6, 28, 12);
    else return;
    disc(250, 150, 2, 0);
}

void orbital_ref(void) {
    if (ast.tab > 9) ast.tab = 0;
    for (;;) {
        uint8_t i, k;
        char head[48], *p;
        page(ast.tab);
        px_fill(0, STATUS_H, LCD_W, LCD_H - STATUS_H, 0xFFFF);
        px_fill(0, STATUS_H, LCD_W, 16, BAR);
        p = put_u(head, (uint16_t)(ast.tab + 1));
        p = put_s(p, "/10 ");
        put_s(p, titles[ast.tab]);
        plat_small(head, 4, STATUS_H + 2, 0xFFFF, BAR);
        plat_small("\x05 \x05 1-9,0 CLEAR", 200, STATUS_H + 2, 0xFFFF, BAR);
        for (i = 0; i < nl; i++) plat_small(pg[i], 4, (uint8_t)(STATUS_H + 20 + i * LH), 0, 0xFFFF);
        shape(ast.tab);
        k = wait_key();
        if (k == K_RIGHT || k == K_ENTER) ast.tab = (uint8_t)((ast.tab + 1) % 10);
        else if (k == K_LEFT) ast.tab = (uint8_t)((ast.tab + 9) % 10);
        else if (k == K_0) ast.tab = 9;
        else if (k == K_1) ast.tab = 0;
        else if (k == K_2) ast.tab = 1;
        else if (k == K_3) ast.tab = 2;
        else if (k == K_4) ast.tab = 3;
        else if (k == K_5) ast.tab = 4;
        else if (k == K_6) ast.tab = 5;
        else if (k == K_7) ast.tab = 6;
        else if (k == K_8) ast.tab = 7;
        else if (k == K_9) ast.tab = 8;
        else if (k == K_CLEAR || k == K_MODE || k == K_ZOOM) return;
    }
}
