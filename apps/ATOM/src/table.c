/* table.c - the periodic table, laid out and driven like PTABLE v2.
 *   arrows move (gaps skipped), ENTER / [TRACE] = INFO pages,
 *   [Y=] / [WINDOW] = OPTIONS (color by family, metal type, block, state),
 *   [ZOOM] = LIST (find by number, symbol or name; a digit or ALPHA starts
 *   it too), [GRAPH] / CLEAR = back to the ATOM line. */
#include <string.h>
#include "atom.h"
#include "plat.h"

#define TX0 6           /* period 1, group 1 cell */
#define TY0 34
#define PITCH 17        /* cells are 18 px, neighbours share a border */
#define FX0 57          /* f-series rows */
#define FY0 161
#ifndef C_WHITE
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000
#endif

static const uint16_t fam_fill[11] = {
    RGB(246, 194, 57), RGB(246, 234, 49), RGB(230, 202, 230), RGB(189, 210, 230), RGB(139, 218, 189),
    RGB(131, 226, 24), RGB(139, 234, 238), RGB(156, 194, 255), RGB(246, 194, 197), RGB(213, 202, 246), C_WHITE};
static const uint16_t fam_edge[11] = {
    RGB(148, 113, 32), RGB(148, 137, 24), RGB(139, 121, 131), RGB(115, 129, 139), RGB(82, 129, 106),
    RGB(74, 129, 16), RGB(82, 137, 139), RGB(90, 113, 156), RGB(148, 113, 115), RGB(123, 121, 148), RGB(164, 161, 164)};
/* saturated family colors: INFO header, Find chips, swatches */
const uint16_t fam_strong[10] = {
    RGB(255, 0, 0), RGB(255, 137, 32), RGB(0, 145, 255), RGB(197, 194, 197), RGB(0, 153, 0),
    RGB(255, 250, 0), RGB(0, 0, 255), RGB(0, 0, 131), RGB(255, 0, 255), RGB(180, 32, 0)};
static const uint16_t view_pal[3][4] = {
    {RGB(0, 145, 255), RGB(255, 250, 0), RGB(0, 153, 0), RGB(255, 250, 0)},       /* metal type by CAT_ */
    {RGB(255, 0, 0), RGB(255, 250, 0), RGB(0, 145, 255), RGB(0, 153, 0)},         /* s p d f */
    {RGB(197, 194, 197), RGB(0, 0, 255), RGB(255, 0, 0), RGB(230, 226, 230)}};     /* state */
static const char *const view_title[4] = {"", "METAL TYPE", "BLOCK", "STATE"};
static const char *const view_item[4] = {"Element family", "Metal type", "Block (s p d f)", "State (room temp)"};
static const char *const t_tabs[5] = {"OPTIONS", "OPTIONS", "LIST", "INFO", "BACK"};

/* ---- helpers shared with info.c, iso.c, orbital.c ---- */
uint8_t wait_key(void) {
    uint8_t k;
    while (!(k = plat_getcsc())) ;
    return k;
}

/* "iron" -> "Iron" */
char *el_name(char *d, uint8_t z) {
    char *e = put_s(d, elements[z].name);
    if (*d >= 'a' && *d <= 'z') *d = (char)(*d - 32);
    return e;
}

static uint8_t block_i(uint8_t z) {
    char b = el_block(z);
    return b == 's' ? 0 : b == 'p' ? 1 : b == 'd' ? 2 : 3;
}

static void cell_xy(uint8_t z, uint16_t *x, uint16_t *y) {
    uint8_t g = el_group(z);
    if (g) {
        *x = (uint16_t)(TX0 + PITCH * (g - 1));
        *y = (uint16_t)(TY0 + PITCH * (el_period(z) - 1));
    } else {
        *x = (uint16_t)(FX0 + PITCH * (z - (z < 89 ? 57 : 89)));
        *y = (uint16_t)(FY0 + (z < 89 ? 0 : PITCH));
    }
}

static void cell(uint8_t z, bool sel) {
    uint16_t x, y, fill, edge = RGB(49, 48, 49);
    cell_xy(z, &x, &y);
    if (!ast.view) {
        uint8_t f = z >= 113 ? 10 : el_family(z);
        fill = fam_fill[f];
        edge = fam_edge[f];
    } else {
        fill = view_pal[ast.view - 1][ast.view == 1 ? elements[z].cat : ast.view == 2 ? block_i(z) : el_state(z)];
    }
    ui_fill(x, y, 18, 18, edge);
    ui_fill((uint16_t)(x + 1), (uint16_t)(y + 1), 16, 16, fill);
    if (sel) {
        uint16_t k = RGB(32, 32, 32);
        ui_fill((uint16_t)(x + 2), (uint16_t)(y + 2), 14, 2, k);
        ui_fill((uint16_t)(x + 2), (uint16_t)(y + 14), 14, 2, k);
        ui_fill((uint16_t)(x + 2), (uint16_t)(y + 2), 2, 14, k);
        ui_fill((uint16_t)(x + 14), (uint16_t)(y + 2), 2, 14, k);
    }
}

/* name, symbol and number in the gap above groups 3-12 */
static void readout(uint8_t z) {
    char b[16];
    uint16_t w;
    ui_fill(40, TY0, 170, 51, C_WHITE);
    el_name(b, z);
    w = ui_text_width(b, UI_LARGE);
    ui_text(b, (uint16_t)(125 - w / 2), TY0 + 2, UI_LARGE, C_BLACK, C_WHITE);
    ui_text(elements[z].sym, 75, TY0 + 24, UI_LARGE, C_BLACK, C_WHITE);
    put_u(b, z);
    ui_text(b, (uint16_t)(177 - ui_text_width(b, UI_LARGE)), TY0 + 24, UI_LARGE, C_BLACK, C_WHITE);
}

static void draw_table(void) {
    uint8_t z;
    ui_frame("PERIODIC TABLE", view_title[ast.view], t_tabs);
    for (z = 1; z <= NUM_ELEMENTS; z++) cell(z, z == ast.tz);
    /* the f-series bracket under group 3 */
    ui_fill(47, 128, 2, 61, RGB(49, 48, 49));
    ui_fill(47, FY0 + 8, 10, 2, RGB(49, 48, 49));
    ui_fill(47, FY0 + PITCH + 8, 10, 2, RGB(49, 48, 49));
    readout(ast.tz);
}

/* grid row 0-6 = periods, 7-8 = f-series; column 0-17 */
static void rc(uint8_t z, int8_t *r, int8_t *c) {
    uint8_t g = el_group(z);
    if (g) { *r = (int8_t)(el_period(z) - 1); *c = (int8_t)(g - 1); }
    else { *r = z < 89 ? 7 : 8; *c = (int8_t)(3 + z - (z < 89 ? 57 : 89)); }
}

/* nearest element from z stepping (dr, dc); 0 = none. DOWN stays out of
 * the f rows, UP from them goes back into the table (as in PTABLE). */
static uint8_t step(uint8_t z, int8_t dr, int8_t dc) {
    int8_t r, c;
    rc(z, &r, &c);
    for (;;) {
        uint8_t n;
        r = (int8_t)(r + dr);
        c = (int8_t)(c + dc);
        if (r < 0 || r > 8 || c < 0 || c > 17 || (dr > 0 && r == 7)) return 0;
        for (n = 1; n <= NUM_ELEMENTS; n++) {
            int8_t nr, nc;
            rc(n, &nr, &nc);
            if (nr == r && nc == c) return n;
        }
    }
}

/* ---- OPTIONS ---- */
static void options(void) {
    uint8_t i = ui_pick_at("OPTIONS", view_item, 4, ast.view);
    if (i < 4) ast.view = i;
}

/* ---- LIST / Find ---- */
#define FIND_MAX 12
#define FIND_ROWS 6
#define FIND_X 28
#define FIND_Y 26
#define FIND_W 264
#define FIND_H 189

static char q[FIND_MAX + 1];
static uint8_t hits[NUM_ELEMENTS], nhits;

static bool prefix(const char *s, const char *p) {
    for (; *p; s++, p++) {
        char a = *s, b = *p;
        if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
        if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
        if (a != b) return false;
    }
    return true;
}

/* numbers: Z starts with the digits; letters: symbol prefix, then name prefix */
static void search(void) {
    uint8_t z, pass;
    nhits = 0;
    if (!q[0]) return;
    for (pass = 0; pass < 2; pass++)
        for (z = 1; z <= NUM_ELEMENTS; z++) {
            char b[4];
            bool m;
            if (q[0] >= '0' && q[0] <= '9') {
                put_u(b, z);
                m = !pass && prefix(b, q);
            } else {
                m = pass ? prefix(elements[z].name, q) && !prefix(elements[z].sym, q) : prefix(elements[z].sym, q);
            }
            if (m) hits[nhits++] = z;
        }
}

static void chip(uint8_t z, uint16_t x, uint8_t y) {
    uint8_t f = el_family(z);
    uint16_t bg = fam_strong[f], fg = f == 6 || f == 7 || f == 9 || f == 0 || f == 4 || f == 8 ? C_WHITE : C_BLACK;
    uint16_t w = ui_text_width(elements[z].sym, UI_BOLD);
    ui_fill(x, y, 21, 15, bg);
    ui_text(elements[z].sym, (uint16_t)(x + (21 - w) / 2), (uint8_t)(y + 2), UI_BOLD | UI_TRANS, fg, bg);
}

static void find_draw(bool letters, uint8_t sel, uint8_t top) {
    uint8_t i;
    uint16_t w;
    ui_fill(FIND_X + FIND_W, FIND_Y + 3, 3, FIND_H, C_SHADOW);
    ui_fill(FIND_X + 3, FIND_Y + FIND_H, FIND_W, 3, C_SHADOW);
    ui_box(FIND_X, FIND_Y, FIND_W, FIND_H, C_BORDER);
    ui_fill(FIND_X + 1, FIND_Y + 1, FIND_W - 2, 20, C_NAVY);
    ui_text("Find element", FIND_X + 8, FIND_Y + 3, UI_LARGE | UI_BOLD | UI_TRANS, C_WHITE, C_NAVY);
    w = ui_text_width("CLEAR: cancel", 0);
    ui_text("CLEAR: cancel", (uint16_t)(FIND_X + FIND_W - 6 - w), FIND_Y + 6, 0, C_WHITE, C_NAVY);
    ui_fill(FIND_X + 1, FIND_Y + 21, FIND_W - 2, FIND_H - 22, C_WHITE);
    ui_box(FIND_X + 10, FIND_Y + 26, FIND_W - 20, 22, C_BORDER);
    w = ui_text(q, FIND_X + 14, FIND_Y + 29, UI_LARGE, C_BLACK, C_WHITE);
    ui_fill((uint16_t)(FIND_X + 15 + w), FIND_Y + 29, 1, 16, C_BLACK);         /* caret */
    ui_fill(FIND_X + FIND_W - 26, FIND_Y + 30, 14, 14, letters ? C_NAVY : RGB(230, 226, 230));
    ui_text(letters ? "A" : "1", FIND_X + FIND_W - 22, FIND_Y + 31, UI_BOLD | UI_TRANS, letters ? C_WHITE : C_BLACK,
            C_NAVY);
    ui_text("Number, symbol or name \x0C ALPHA = letters", FIND_X + 10, FIND_Y + 50, 0, C_GREY, C_WHITE);
    ui_fill(FIND_X + 1, FIND_Y + 64, FIND_W - 2, 1, C_GREY);
    if (q[0] && !nhits) ui_text("No match", FIND_X + 14, FIND_Y + 72, UI_LARGE, C_GREY, C_WHITE);
    for (i = 0; i < FIND_ROWS && top + i < nhits; i++) {
        uint8_t z = hits[top + i], y = (uint8_t)(FIND_Y + 68 + 18 * i);
        uint16_t bg = top + i == sel ? C_SELECT : C_WHITE;
        char b[16];
        ui_fill(FIND_X + 1, y, FIND_W - 2, 18, bg);
        chip(z, FIND_X + 10, (uint8_t)(y + 1));
        el_name(b, z);
        ui_text(b, FIND_X + 40, (uint8_t)(y + 1), UI_LARGE, C_BLACK, bg);
        put_u(b, z);
        ui_text(b, (uint16_t)(FIND_X + FIND_W - 14 - ui_text_width(b, UI_LARGE)), (uint8_t)(y + 1), UI_LARGE, C_BLACK, bg);
    }
    if (nhits > FIND_ROWS) {                      /* scrollbar */
        uint16_t h = (uint16_t)(18 * FIND_ROWS);
        ui_fill(FIND_X + FIND_W - 6, FIND_Y + 68, 3, h, RGB(230, 226, 230));
        ui_fill(FIND_X + FIND_W - 6, (uint16_t)(FIND_Y + 68 + h * top / nhits), 3, (uint16_t)(h * FIND_ROWS / nhits), C_GREY);
    }
}

/* returns the picked Z, 0 = cancelled; `first` = key that opened it */
static uint8_t find(uint8_t first) {
    bool letters = false;
    uint8_t sel = 0, top = 0, k = first;
    q[0] = 0;
    for (;;) {
        size_t n = strlen(q);
        char c = 0;
        if (k == K_ALPHA) letters = !letters;
        else if (k == K_CLEAR) return 0;
        else if (k == K_DEL) { if (n) q[n - 1] = 0; }
        else if (k == K_UP) { if (sel) sel--; }
        else if (k == K_DOWN) { if (sel + 1 < nhits) sel++; }
        else if (k == K_ENTER) { if (nhits) return hits[sel]; }
        else if (k != K_ZOOM) c = entry_char(k, LOCK_NUM, letters, false, SMART_NONE);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')) {
            if (!n && c >= 'a') c = (char)(c - 32);          /* "Fe" */
            if (n < FIND_MAX) { q[n] = c; q[n + 1] = 0; }
            sel = 0;
        }
        if (k != K_UP && k != K_DOWN) search();
        if (sel < top) top = sel;
        if (sel >= top + FIND_ROWS) top = (uint8_t)(sel - FIND_ROWS + 1);
        find_draw(letters, sel, top);
        k = wait_key();
    }
}

void table_view(void) {
    if (ast.tz < 1 || ast.tz > NUM_ELEMENTS) ast.tz = 1;
    if (ast.view > 3) ast.view = 0;
    draw_table();
    for (;;) {
        uint8_t k = wait_key(), n = 0;
        switch (k) {
        case K_LEFT: n = step(ast.tz, 0, -1); break;
        case K_RIGHT: n = step(ast.tz, 0, 1); break;
        case K_UP: n = step(ast.tz, -1, 0); break;
        case K_DOWN: n = step(ast.tz, 1, 0); break;
        case K_YEQU: case K_WINDOW:
            options();
            draw_table();
            break;
        case K_ENTER: case K_TRACE:
            if (info_pages(ast.tz)) return;
            draw_table();
            break;
        case K_CLEAR: case K_GRAPH: case K_MODE:
            return;
        default: {
            char c = entry_char(k, LOCK_NUM, false, false, SMART_NONE);
            if (k == K_ZOOM || k == K_ALPHA || (c >= '0' && c <= '9')) {
                n = find(k);
                draw_table();
                if (n == ast.tz) n = 0;
            }
        }
        }
        if (n) {
            cell(ast.tz, false);
            ast.tz = n;
            cell(n, true);
            readout(n);
        }
    }
}
