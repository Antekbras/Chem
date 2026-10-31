/* ui_ce.c - drawing with the OS home-screen font (calculator only).
 * Large text goes through os_SetCursorPos/os_PutStrLine so the grid is the
 * home screen's own 26x10 grid. Pixel constants below are only used for the
 * small grey decimal and the field bar; tune them on hardware if needed. */
#ifdef __TICE__
#include <string.h>
#include <time.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include <ti/ui.h>
#include <ti/flags.h>
#include <sys/lcd.h>
#include "ui.h"

#define ROW_Y0   30      /* first text row under the status bar */
#define ROW_PX   21      /* pixel height of a home-screen row */
#define COL_PX   12      /* pixel width of a large-font cell */
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_GREY   0x7BEF
#define C_BAR    0x1C3F  /* OS menu blue */

static void colors(uint16_t fg, uint16_t bg) {
    os_TextFGColor = fg;
    os_TextBGColor = bg;
}

static void put_at(uint8_t row, uint8_t col, const char *s) {
    os_SetCursorPos(row, col);
    os_PutStrLine(s);
}

/* ---- cursor geometry, measured on the real screen ----
 * The cursor is drawn by inverting the pixels of its cell directly in
 * VRAM, so it shows no matter how the OS treats text colors. The cell
 * size and position are measured once by drawing 'H' in cells (0,0) and
 * (1,1) and finding the dark pixels. */
#define LCD_W 320
#define LCD_H 240
static uint16_t cell_x0 = 0, cell_y0 = ROW_Y0, cell_w = COL_PX, cell_h = ROW_PX;

typedef struct { int16_t x0, y0, x1, y1; } box_t;

static bool ink_box(box_t *b) {
    uint16_t *vram = (uint16_t *)lcd_Ram;
    int16_t x, y;
    b->x0 = b->y0 = 0x7FFF;
    b->x1 = b->y1 = -1;
    for (y = ROW_Y0; y < ROW_Y0 + 3 * ROW_PX + 10 && y < LCD_H; y++)
        for (x = 0; x < 4 * COL_PX + 10; x++)
            if (vram[y * LCD_W + x] != 0xFFFF) {
                if (x < b->x0) b->x0 = x;
                if (x > b->x1) b->x1 = x;
                if (y < b->y0) b->y0 = y;
                if (y > b->y1) b->y1 = y;
            }
    return b->x1 >= 0;
}

static void calibrate(void) {
    box_t a, b;
    int16_t pw, ph, gw, gh;
    put_at(0, 0, "H");
    if (!ink_box(&a)) return;
    put_at(0, 0, " ");
    put_at(1, 1, "H");
    if (!ink_box(&b)) { put_at(1, 1, " "); return; }
    put_at(1, 1, " ");
    pw = (int16_t)(b.x0 - a.x0);
    ph = (int16_t)(b.y0 - a.y0);
    if (pw < 8 || pw > 16 || ph < 12 || ph > 26) return;   /* keep defaults */
    gw = (int16_t)(a.x1 - a.x0 + 1);
    gh = (int16_t)(a.y1 - a.y0 + 1);
    cell_w = (uint16_t)pw;
    cell_h = (uint16_t)(ph - 1);
    cell_x0 = (uint16_t)(a.x0 > (pw - gw) / 2 ? a.x0 - (pw - gw) / 2 : 0);
    cell_y0 = (uint16_t)(a.y0 - (ph - gh) / 2);
    if (cell_y0 < ROW_Y0) cell_y0 = ROW_Y0;
}

static void invert_cell(uint8_t row, uint8_t col) {
    uint16_t *vram = (uint16_t *)lcd_Ram;
    uint16_t x0 = (uint16_t)(cell_x0 + col * cell_w), y0 = (uint16_t)(cell_y0 + row * (cell_h + 1));
    uint16_t x, y;
    for (y = y0; y < y0 + cell_h && y < LCD_H; y++)
        for (x = x0; x < x0 + cell_w && x < LCD_W; x++)
            vram[y * LCD_W + x] ^= 0xFFFF;
}

void ui_begin(void) {
    static bool measured;
    os_FontSelect(os_LargeFont);
    colors(C_BLACK, C_WHITE);
    os_ClrHomeFull();
    if (!measured) { calibrate(); measured = true; }
}

void ui_end(void) {
    colors(C_BLACK, C_WHITE);
    os_ClrHomeFull();
}

static void small_text(const char *s, uint16_t x, uint8_t y, uint16_t fg, uint16_t bg) {
    os_FontSelect(os_SmallFont);
    os_SetDrawFGColor(fg);
    os_SetDrawBGColor(bg);
    os_FontDrawText(s, x, y);
    os_FontSelect(os_LargeFont);
}

void ui_draw(const screen_t *s, const entry_t *e, uint8_t rows) {
    srow_t r[UI_ROWS];
    uint8_t n, i, crow, ccol;
    char line[UI_COLS + 1];
    n = ui_layout(s, e, rows, r, &crow, &ccol);
    colors(C_BLACK, C_WHITE);
    for (i = 0; i < rows; i++) {
        memset(line, ' ', UI_COLS);
        line[UI_COLS] = 0;
        if (i < n) memcpy(line + r[i].col, r[i].text, r[i].len);
        put_at(i, 0, line);
        if (i < n && r[i].kind == ROW_OUT && r[i].small && r[i].small[0]) {
            os_FontSelect(os_SmallFont);
            if (os_FontGetWidth(r[i].small) + 6 < (uint24_t)r[i].col * COL_PX)
                small_text(r[i].small, 2, (uint8_t)(ROW_Y0 + i * ROW_PX + 4), C_GREY, C_WHITE);
            os_FontSelect(os_LargeFont);
        }
    }
}

static void cursor(const entry_t *e, uint8_t row, uint8_t col, bool on) {
    char c[2];
    if (row == 0xFF) return;
    c[1] = 0;
    c[0] = e->cur < e->len ? e->buf[e->cur] : ' ';
    if (on) {
        /* glyph shown inverted: block, A (letter-lock), ^ (2nd), a (ALPHA), _ (insert) */
        if (e->second) c[0] = '^';
        else if (e->alpha) c[0] = 'a';
        else if (e->lock == LOCK_LET) c[0] = 'A';
        else if (e->ins) c[0] = '_';
        else c[0] = ' ';
    }
    colors(C_BLACK, C_WHITE);
    put_at(row, col, c);
    if (on) invert_cell(row, col);
}

uint8_t ui_getkey(const screen_t *s, const entry_t *e, uint8_t rows) {
    srow_t r[UI_ROWS];
    uint8_t k, crow, ccol;
    bool on = true;
    clock_t t = clock();
    ui_layout(s, e, rows, r, &crow, &ccol);
    cursor(e, crow, ccol, on);
    while (!(k = os_GetCSC())) {
        if (clock() - t > CLOCKS_PER_SEC / 2) {
            on = !on;
            cursor(e, crow, ccol, on);
            t = clock();
        }
    }
    cursor(e, crow, ccol, false);
    return k;
}

uint8_t ui_show_work(const char *title, const work_t *w) {
    uint8_t top = 0, i, k;
    char line[UI_COLS + 1];
    for (;;) {
        colors(C_WHITE, C_BAR);
        memset(line, ' ', UI_COLS);
        line[UI_COLS] = 0;
        memcpy(line, title, strlen(title) > UI_COLS ? UI_COLS : strlen(title));
        put_at(0, 0, line);
        colors(C_BLACK, C_WHITE);
        for (i = 1; i < UI_ROWS; i++) {
            uint8_t j = (uint8_t)(top + i - 1);
            memset(line, ' ', UI_COLS);
            if (j < w->n) memcpy(line, w->line[j], strlen(w->line[j]));
            put_at(i, 0, line);
        }
        while (!(k = os_GetCSC())) ;
        if (k == sk_Up && top) top--;
        else if (k == sk_Down && top + UI_ROWS - 1 < w->n) top++;
        else return k;
    }
}

void ui_fields(const char *const labels[5], uint8_t sel) {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        uint16_t x = (uint16_t)(i * 64);
        bool hi = i == sel;
        char cell[12];
        uint8_t len = 0;
        if (labels[i]) {
            len = (uint8_t)strlen(labels[i]);
            if (len > 10) len = 10;
            memcpy(cell, labels[i], len);
        }
        cell[len] = 0;
        small_text("          ", (uint16_t)(x + 1), 224, C_WHITE, hi ? C_BLACK : C_BAR);
        small_text(cell, (uint16_t)(x + 4), 224, C_WHITE, hi ? C_BLACK : C_BAR);
    }
}
#endif
