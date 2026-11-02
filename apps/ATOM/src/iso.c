/* iso.c - isotope form ([WINDOW]): rows of mass and abundance, average
 * atomic mass at the bottom. Blank = solve: average mass, one missing
 * abundance (100 - the rest), one missing mass, or two missing abundances
 * from the average. Abundances in % or as fractions. Recomputes on every
 * key; [TRACE] shows the work, [GRAPH] goes back. */
#include <string.h>
#include "atom.h"
#include "plat.h"

#define NF (ISO_ROWS * 2 + 1)
#define F_AVG (ISO_ROWS * 2)
#define FW ISO_LEN

static num_t val[NF];
static uint8_t have[NF];        /* 1 typed, 2 solved */
static char msg[WORK_COLS + 1];
static work_t iw;

static bool parse(const char *s, num_t *n) {
    uint8_t k;
    if (!*s) return false;
    k = num_parse(s, n);
    return k && !s[k];
}

static num_t P[ISO_ROWS];                  /* abundances in % */
static const num_t N100 = {100, SF_EXACT, LSD_EXACT}, N0 = {0, SF_EXACT, LSD_EXACT};

enum { O_ADD, O_SUB, O_MUL, O_DIV };
/* r = a op b with sig figs; pointers keep the call sites small */
static void calc(num_t *r, const num_t *a, uint8_t o, const num_t *b) {
    num_t x = *a, y = *b;
    switch (o) {
    case O_ADD: *r = num_add(x, y, false); break;
    case O_SUB: *r = num_sub(x, y, false); break;
    case O_MUL: *r = num_mul(x, y, false); break;
    default: *r = num_div(x, y, false); break;
    }
}

static void add_n(const char *a, const num_t *n, const char *b) {
    char t[32];
    num_fmt_sci(*n, t);
    work_add(&iw, a, t, b, NULL);
}

/* solved abundance of `row`, given in % */
static void set_p(uint8_t row, const num_t *pct, bool frac) {
    uint8_t f = (uint8_t)(2 * row + 1);
    P[row] = *pct;
    if (frac) calc(&val[f], pct, O_DIV, &N100);
    else val[f] = *pct;
    have[f] = 2;
}

static void solve(void) {
    uint8_t i, used = 0, um = 0, up = 0, km = 0, kp = 0, kp2 = 0;
    bool frac = true, any_p = false;
    num_t sp, smp, t, x, y;
    work_clear(&iw);
    msg[0] = 0;
    for (i = 0; i < NF; i++) {
        have[i] = 0;
        if (ast.iso[i][0]) {
            if (!parse(ast.iso[i], &val[i])) { strcpy(msg, "not a number"); return; }
            have[i] = 1;
        }
    }
    for (i = 0; i < ISO_ROWS; i++) {
        uint8_t m = (uint8_t)(2 * i);
        if (!have[m] && !have[m + 1]) continue;
        used++;
        if (!have[m]) { um++; km = i; }
        if (!have[m + 1]) { if (up++) kp2 = i; else kp = i; }
        else { any_p = true; if (val[m + 1].v > 1) frac = false; }
    }
    if (!used) { strcpy(msg, "blank = solve"); return; }
    if (!any_p) frac = false;
    /* sums over the known abundances */
    sp = smp = N0;
    for (i = 0; i < ISO_ROWS; i++) {
        uint8_t m = (uint8_t)(2 * i);
        if (!have[m + 1]) continue;
        if (frac) calc(&P[i], &val[m + 1], O_MUL, &N100);
        else P[i] = val[m + 1];
        calc(&sp, &sp, O_ADD, &P[i]);
        if (have[m]) { calc(&x, &val[m], O_MUL, &P[i]); calc(&smp, &smp, O_ADD, &x); }
    }
    /* one missing abundance: 100% minus the rest */
    if (up == 1 && (um == 0 || km == kp || have[F_AVG])) {
        calc(&t, &N100, O_SUB, &sp);
        add_n("% = 100 - ", &sp, NULL);
        add_n("  = ", &t, " %");
        set_p(kp, &t, frac);
        sp = N100;
        if (have[2 * kp]) { calc(&x, &val[2 * kp], O_MUL, &t); calc(&smp, &smp, O_ADD, &x); }
        up = 0;
    }
    if (!have[F_AVG]) {
        if (um || up) { strcpy(msg, um ? "need all masses" : "need avg mass"); return; }
        work_add(&iw, "avg = sum(mass x %)/100", NULL, NULL, NULL);
        for (i = 0; i < ISO_ROWS; i++) {
            if (!have[2 * i]) continue;
            calc(&x, &val[2 * i], O_MUL, &P[i]);
            add_n("+ ", &x, "/100");
        }
        if (FP_FABS(sp.v - 100) > 0.05) {
            char *p = put_s(msg, "% sum ");
            num_fmt_plain(sp, p);
            strcat(msg, ": normalized");
            add_n("sum % = ", &sp, " (not 100)");
            calc(&t, &smp, O_DIV, &sp);
        } else {
            calc(&t, &smp, O_DIV, &N100);
        }
        val[F_AVG] = t;
        have[F_AVG] = 2;
        add_n("avg = ", &t, " u");
        return;
    }
    /* average known */
    calc(&t, &val[F_AVG], O_MUL, &N100);
    if (um == 1 && up == 0) {
        work_add(&iw, "avg x 100 = sum(mass x %)", NULL, NULL, NULL);
        add_n("mass = (", &t, NULL);
        add_n("  - ", &smp, ")");
        add_n("  / ", &P[km], NULL);
        calc(&x, &t, O_SUB, &smp);
        calc(&val[2 * km], &x, O_DIV, &P[km]);
        have[2 * km] = 2;
        add_n("  = ", &val[2 * km], " u");
    } else if (um == 0 && up == 2) {
        /* x + y = R and Ma x + Mb y = 100 avg - known */
        num_t R, T;
        calc(&R, &N100, O_SUB, &sp);
        calc(&T, &t, O_SUB, &smp);
        calc(&x, &R, O_MUL, &val[2 * kp2]);
        calc(&x, &T, O_SUB, &x);
        calc(&y, &val[2 * kp], O_SUB, &val[2 * kp2]);
        calc(&x, &x, O_DIV, &y);
        calc(&y, &R, O_SUB, &x);
        work_add(&iw, "x + y = 100 - known %", NULL, NULL, NULL);
        work_add(&iw, "Ma x + Mb y = 100 avg", NULL, NULL, NULL);
        work_add(&iw, "x = (100avg - Mb(x+y))", NULL, NULL, NULL);
        work_add(&iw, "    / (Ma - Mb)", NULL, NULL, NULL);
        add_n("x = ", &x, " %");
        add_n("y = ", &y, " %");
        set_p(kp, &x, frac);
        set_p(kp2, &y, frac);
    } else if (um || up) {
        strcpy(msg, "too many blanks");
    } else {
        char *p = put_s(msg, "from data: ");
        calc(&x, &smp, O_DIV, &sp);
        num_fmt_plain(x, p);
        add_n("avg = ", &x, " u");
    }
}

static void field_text(uint8_t i, char *out) {
    if (have[i] == 2) {
        char a[32], b[32];
        num_fmt_answer(val[i], a, b);
        out[0] = '=';
        strncpy(out + 1, a, FW - 1);
        out[FW] = 0;
    } else {
        strcpy(out, ast.iso[i]);
    }
}

#define ROW_Y(i) (uint8_t)(52 + 26 * (i))
static const char *const iso_tabs[5] = {"", "", "", "WORK", "BACK"};

/* one input box, like PTABLE's Find box; the selected one is light blue */
static void box(uint8_t i, uint16_t x, uint8_t y, bool sel) {
    char f[16];
    uint16_t bg = sel ? C_SELECT : C_WHITE, w;
    ui_box(x, y, 118, 22, sel ? C_BLACK : C_BORDER);
    ui_fill((uint16_t)(x + 1), (uint8_t)(y + 1), 116, 20, bg);
    field_text(i, f);
    w = ui_text(f, (uint16_t)(x + 4), (uint8_t)(y + 3), UI_LARGE, have[i] == 2 ? RGB(0, 90, 200) : C_BLACK, bg);
    if (sel) ui_fill((uint16_t)(x + 5 + w), (uint8_t)(y + 3), 1, 16, C_BLACK);
}

static void draw(void) {
    uint8_t i, sel = ast.irow < ISO_ROWS ? (uint8_t)(2 * ast.irow + ast.icol) : F_AVG;
    char a[8];
    ui_fill(0, 30, LCD_W, 190, C_WHITE);
    ui_text("Isotope", 8, 36, 0, C_GREY, C_WHITE);
    ui_text("Mass (u)", 64, 36, 0, C_GREY, C_WHITE);
    ui_text("Abundance (%)", 192, 36, 0, C_GREY, C_WHITE);
    for (i = 0; i < ISO_ROWS; i++) {
        uint8_t m = (uint8_t)(2 * i), y = ROW_Y(i);
        if (have[m]) put_u(a, (uint16_t)(val[m].v + 0.5));   /* label: mass number */
        else { a[0] = (char)('1' + i); a[1] = 0; }
        ui_fill(0, y, 58, 22, C_WHITE);
        ui_text(a, 12, (uint8_t)(y + 3), UI_LARGE | UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
        box(m, 60, y, sel == m);
        box((uint8_t)(m + 1), 188, y, sel == m + 1);
    }
    ui_fill(0, ROW_Y(ISO_ROWS) - 3, LCD_W, 1, C_BORDER);
    ui_text("Average atomic mass", 8, (uint8_t)(ROW_Y(ISO_ROWS) + 5), UI_BOLD | UI_TRANS, C_BLACK, C_WHITE);
    box(F_AVG, 188, ROW_Y(ISO_ROWS), sel == F_AVG);
    ui_text(msg[0] ? msg : "Leave one box blank to solve it", 8, 205, 0, msg[0] ? RGB(200, 0, 0) : C_GREY,
            C_WHITE);
}

void iso_form(void) {
    bool second = false;
    ui_frame("ISOTOPES", "AVERAGE MASS", iso_tabs);
    solve();
    for (;;) {
        uint8_t k, sel;
        char *f;
        size_t n;
        draw();
        k = wait_key();
        sel = ast.irow < ISO_ROWS ? (uint8_t)(2 * ast.irow + ast.icol) : F_AVG;
        f = ast.iso[sel];
        n = strlen(f);
        if (second) {
            second = false;
            if (k == K_MODE) return;                 /* 2nd+QUIT */
            if (k == K_COMMA && n < ISO_LEN) { f[n] = CH_EE; f[n + 1] = 0; }
        } else if (k == K_2ND) {
            second = true;
        } else if (k == K_CLEAR) {
            if (!n) return;
            f[0] = 0;
        } else if (k == K_DEL) {
            if (n) f[n - 1] = 0;
        } else if (k == K_UP) {
            ast.irow = ast.irow ? (uint8_t)(ast.irow - 1) : ISO_ROWS;
        } else if (k == K_DOWN) {
            ast.irow = ast.irow < ISO_ROWS ? (uint8_t)(ast.irow + 1) : 0;
        } else if (k == K_LEFT || k == K_RIGHT) {
            ast.icol ^= 1;
        } else if (k == K_ENTER) {
            /* mass -> % -> next row; ENTER on a blank row goes to the average */
            if (ast.irow == ISO_ROWS) ast.irow = 0;
            else if (!ast.icol && !n && !ast.iso[sel + 1][0]) ast.irow = ISO_ROWS;
            else if (!ast.icol) ast.icol = 1;
            else { ast.icol = 0; ast.irow++; }
            if (ast.irow == ISO_ROWS) ast.icol = 0;
        } else if (k == K_GRAPH) {
            return;
        } else if (k == K_TRACE) {
            if (iw.n) { ui_show_work("ISOTOPES", &iw); ui_frame("ISOTOPES", "AVERAGE MASS", iso_tabs); }
        } else {
            char c = entry_char(k, LOCK_NUM, false, false, SMART_NONE);
            if (((c >= '0' && c <= '9') || c == '.' || c == CH_NEG) && n < ISO_LEN) {
                f[n] = c;
                f[n + 1] = 0;
            }
        }
        solve();
    }
}
