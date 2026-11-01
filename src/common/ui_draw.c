/* ui_draw.c - home-screen drawing on top of plat.h (calculator and host).
 *
 * Every big character is drawn by the OS's own home-screen text routine
 * in its own grid cell, so the look is the home screen's. Before drawing,
 * the cell's whole box is filled, and the box is exactly the area the OS
 * paints for one character: it is measured once by painting the screen
 * black and letting the OS print spaces in cells (0,0) and (1,1). So a
 * redraw never depends on what was there before, and the cursor (the
 * same box, inverted) is fully removed by redrawing the cell. `shown`
 * mirrors what is on screen so a keypress only redraws changed cells. */
#include <string.h>
#include "ui.h"
#include "plat.h"
#include "keys.h"

#define C_BLACK 0x0000
#define C_WHITE 0xFFFF
#define C_GREY  0x7BEF
#define C_BAR   0xD304   /* blue, BGR565 (same as before) */

/* grid: cell (r,c) box = (gx0 + c*gpw, gy0 + r*gph), size gbw x gbh */
static uint16_t gx0 = 0, gy0 = STATUS_H, gpw = 12, gph = 21, gbw = 12, gbh = 21;
static bool measured;

static char shown[UI_ROWS][UI_COLS];      /* 0 = unknown */
static char shown_small[UI_ROWS][UI_SMALL];

static void fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint16_t *v = plat_vram();
    uint16_t i, j;
    for (j = y; j < y + h && j < LCD_H; j++) {
        uint16_t *p = v + (uint32_t)j * LCD_W + x;
        for (i = x; i < x + w && i < LCD_W; i++) *p++ = color;
    }
}

typedef struct { int16_t x0, y0, x1, y1; } box_t;

/* bounding box of white pixels in the scan area */
static bool white_box(box_t *b) {
    const uint16_t *v = plat_vram();
    int16_t x, y;
    b->x0 = b->y0 = 0x7FFF;
    b->x1 = b->y1 = -1;
    for (y = STATUS_H; y < STATUS_H + 80; y++)
        for (x = 0; x < 64; x++)
            if (v[y * LCD_W + x] == C_WHITE) {
                if (x < b->x0) b->x0 = x;
                if (x > b->x1) b->x1 = x;
                if (y < b->y0) b->y0 = y;
                if (y > b->y1) b->y1 = y;
            }
    return b->x1 >= 0;
}

/* Where does the OS paint cells (0,0) and (1,1)? Leaves the screen clear. */
static void measure(void) {
    box_t a, b;
    int16_t w, h, pw, ph;
    fill(0, STATUS_H, 64, 80, C_BLACK);
    plat_home_put(0, 0, " ");
    if (!white_box(&a)) goto done;
    fill(0, STATUS_H, 64, 80, C_BLACK);
    plat_home_put(1, 1, " ");
    if (!white_box(&b)) goto done;
    w = (int16_t)(a.x1 - a.x0 + 1);
    h = (int16_t)(a.y1 - a.y0 + 1);
    pw = (int16_t)(b.x0 - a.x0);
    ph = (int16_t)(b.y0 - a.y0);
    if (pw < 8 || pw > 16 || ph < 12 || ph > 26 || w > pw || h > ph ||
        a.x0 + UI_COLS * pw > LCD_W || a.y0 + UI_ROWS * ph > LCD_H + (ph - h))
        goto done;                               /* keep defaults */
    gx0 = (uint16_t)a.x0; gy0 = (uint16_t)a.y0;
    gpw = (uint16_t)pw;   gph = (uint16_t)ph;
    gbw = (uint16_t)w;    gbh = (uint16_t)h;
done:
    fill(0, STATUS_H, 64, 80, C_WHITE);
}

static uint16_t cell_x(uint8_t c) { return (uint16_t)(gx0 + c * gpw); }
static uint16_t cell_y(uint8_t r) { return (uint16_t)(gy0 + r * gph); }

/* recolor a cell drawn black on white */
static void recolor(uint8_t r, uint8_t c, uint16_t fg, uint16_t bg) {
    uint16_t *v = plat_vram();
    uint16_t x0 = cell_x(c), y0 = cell_y(r), x, y;
    for (y = y0; y < y0 + gbh && y < LCD_H; y++) {
        uint16_t *p = v + (uint32_t)y * LCD_W + x0;
        for (x = x0; x < x0 + gbw && x < LCD_W; x++, p++) *p = *p == C_WHITE ? bg : fg;
    }
}

static void draw_cell(uint8_t r, uint8_t c, char ch) {
    char s[2];
    fill(cell_x(c), cell_y(r), gbw, gbh, C_WHITE);
    if (ch == ' ') return;
    s[0] = ch; s[1] = 0;
    plat_home_put(r, c, s);
}

static void invert_cell(uint8_t r, uint8_t c) {
    uint16_t *v = plat_vram();
    uint16_t x0 = cell_x(c), y0 = cell_y(r), x, y;
    for (y = y0; y < y0 + gbh && y < LCD_H; y++) {
        uint16_t *p = v + (uint32_t)y * LCD_W + x0;
        for (x = x0; x < x0 + gbw && x < LCD_W; x++) *p++ ^= 0xFFFF;
    }
}

/* unknown rows: shown_small "\1" never matches, so the row's whole strip
 * is repainted (cells and the gaps between them) */
static void forget_row(uint8_t r) {
    memset(shown[r], 0, UI_COLS);
    memset(shown_small[r], 0, UI_SMALL);
    shown_small[r][0] = 1;
}

void ui_invalidate(void) {
    uint8_t r;
    for (r = 0; r < UI_ROWS; r++) forget_row(r);
}

void ui_begin(void) {
    plat_home_reset();
    if (!measured) { measure(); measured = true; }
    memset(shown, ' ', sizeof shown);        /* a cleared screen is all spaces */
    memset(shown_small, 0, sizeof shown_small);
}

void ui_end(void) {
    plat_home_reset();
}

/* small grey text sits at the bottom left of its row, clear of the answer */
static bool small_fits(const char *sm, uint8_t textcol) {
    return sm[0] && plat_small_width(sm) + 4 <= (uint16_t)textcol * gpw &&
           plat_small_height() <= gbh;
}

static void draw_row(uint8_t i, const char *line, const char *sm, uint8_t textcol) {
    uint8_t c;
    bool full = strncmp(sm, shown_small[i], UI_SMALL - 1) != 0;
    /* a row carrying small text is repainted whole, so the small text and
     * the cells under it can never get out of step */
    if (!full && sm[0])
        full = memcmp(line, shown[i], UI_COLS) != 0;
    if (full) {          /* whole strip, gaps between cells included */
        fill(gx0, cell_y(i), (uint16_t)(UI_COLS * gpw), gbh, C_WHITE);
        memset(shown[i], ' ', UI_COLS);
    }
    for (c = 0; c < UI_COLS; c++) {
        if (line[c] == shown[i][c]) continue;
        draw_cell(i, c, line[c]);
        shown[i][c] = line[c];
    }
    if (full) {
        strncpy(shown_small[i], sm, UI_SMALL - 1);
        shown_small[i][UI_SMALL - 1] = 0;
        if (small_fits(sm, textcol))
            plat_small(sm, (uint16_t)(gx0 + 1), (uint8_t)(cell_y(i) + gbh - plat_small_height()),
                       C_GREY, C_WHITE);
    }
}

void ui_draw(const screen_t *s, const entry_t *e, uint8_t rows) {
    srow_t r[UI_ROWS];
    uint8_t n, i, crow, ccol;
    char line[UI_COLS];
    n = ui_layout(s, e, rows, r, &crow, &ccol);
    for (i = 0; i < rows; i++) {
        const char *sm = "";
        uint8_t col = UI_COLS;
        memset(line, ' ', UI_COLS);
        if (i < n) {
            memcpy(line + r[i].col, r[i].text, r[i].len);
            col = r[i].col;
            if (r[i].kind == ROW_OUT && r[i].small) sm = r[i].small;
        }
        draw_row(i, line, sm, col);
    }
}

char ui_cursor_glyph(const entry_t *e) {
    if (e->second) return '^';
    if (e->alpha) return 'a';
    if (e->lock == LOCK_LET) return 'A';
    if (e->ins) return '_';
    return ' ';
}

/* cursor on: the indicator glyph, inverted. off: the cell as `shown` has it */
static void cursor(const entry_t *e, uint8_t row, uint8_t col, bool on) {
    if (row >= UI_ROWS || col >= UI_COLS) return;
    if (!on) {
        char c = shown[row][col];
        draw_cell(row, col, c ? c : ' ');
        if (!c) shown[row][col] = ' ';
        return;
    }
    draw_cell(row, col, ui_cursor_glyph(e));
    invert_cell(row, col);
}

uint8_t ui_getkey(const screen_t *s, const entry_t *e, uint8_t rows) {
    srow_t r[UI_ROWS];
    uint8_t k, crow, ccol;
    bool on = true;
    uint32_t t = plat_ticks();
    ui_layout(s, e, rows, r, &crow, &ccol);
    cursor(e, crow, ccol, true);
    while (!(k = plat_getcsc())) {
        if (plat_ticks() - t > PLAT_TICKS_PER_SEC / 2) {
            on = !on;
            cursor(e, crow, ccol, on);
            t = plat_ticks();
        }
    }
    cursor(e, crow, ccol, false);
    return k;
}

static void text_row(uint8_t row, const char *s, uint16_t fg, uint16_t bg) {
    uint8_t c;
    size_t len = strlen(s);
    fill(gx0, cell_y(row), (uint16_t)(UI_COLS * gpw), gbh, bg);
    for (c = 0; c < UI_COLS && c < len; c++) {
        if (s[c] == ' ') continue;
        draw_cell(row, c, s[c]);
        if (fg != C_BLACK || bg != C_WHITE) recolor(row, c, fg, bg);
    }
}

uint8_t ui_show_work(const char *title, const work_t *w) {
    uint8_t top = 0, i, k;
    ui_invalidate();
    for (;;) {
        text_row(0, title, C_WHITE, C_BAR);
        for (i = 1; i < UI_ROWS; i++) {
            uint8_t j = (uint8_t)(top + i - 1);
            text_row(i, j < w->n ? w->line[j] : "", C_BLACK, C_WHITE);
        }
        while (!(k = plat_getcsc())) ;
        if (k == K_UP) { if (top) top--; }
        else if (k == K_DOWN) { if (top + UI_ROWS - 1 < w->n) top++; }
        else return k;
    }
}

uint8_t ui_pick(const char *title, const char *const *items, uint8_t n) {
    uint8_t top = 0, sel = 0, i, k;
    char row[UI_COLS + 1];
    ui_invalidate();
    for (;;) {
        if (sel < top) top = sel;
        if (sel >= top + UI_ROWS - 1) top = (uint8_t)(sel - (UI_ROWS - 2));
        text_row(0, title, C_WHITE, C_BAR);
        for (i = 1; i < UI_ROWS; i++) {
            uint8_t j = (uint8_t)(top + i - 1);
            row[0] = 0;
            if (j < n) {
                row[0] = j < 9 ? (char)('1' + j) : j == 9 ? '0' : ' ';
                row[1] = ':';
                strncpy(row + 2, items[j], UI_COLS - 2);
                row[UI_COLS] = 0;
            }
            text_row(i, row, j == sel ? C_WHITE : C_BLACK, j == sel ? C_BLACK : C_WHITE);
        }
        while (!(k = plat_getcsc())) ;
        if (k == K_UP) sel = sel ? (uint8_t)(sel - 1) : (uint8_t)(n - 1);
        else if (k == K_DOWN) sel = (uint8_t)(sel + 1 < n ? sel + 1 : 0);
        else if (k == K_ENTER) return sel;
        else {
            static const uint8_t dig[10] = {K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_0};
            for (i = 0; i < 10 && i < n; i++) if (k == dig[i]) return i;
            return 0xFF;
        }
    }
}

void ui_fields(const char *const labels[5], uint8_t sel) {
    uint8_t i, c, r = UI_ROWS - 1;
    fill(gx0, cell_y(r), (uint16_t)(UI_COLS * gpw), gbh, C_BAR);
    for (i = 0; i < 5; i++) {
        const char *l = labels[i] ? labels[i] : "";
        uint16_t bg = i == sel ? C_BLACK : C_BAR;
        size_t len = strlen(l);
        fill(cell_x((uint8_t)(i * 5)), cell_y(r), (uint16_t)(5 * gpw), gbh, bg);
        for (c = 0; c < 5 && c < len; c++) {
            uint8_t col = (uint8_t)(i * 5 + c);
            if (l[c] == ' ') continue;
            draw_cell(r, col, l[c]);
            recolor(r, col, C_WHITE, bg);
        }
    }
    forget_row(r);                  /* repaint when the bar goes away */
}

/* geometry, for tests */
void ui_grid(uint16_t *x0, uint16_t *y0, uint16_t *pw, uint16_t *ph, uint16_t *bw, uint16_t *bh) {
    *x0 = gx0; *y0 = gy0; *pw = gpw; *ph = gph; *bw = gbw; *bh = gbh;
}
