/* ui_compat.c - see ui_compat.h. Empty once the shared helpers exist. */
#include <string.h>
#include "ui_compat.h"
#ifdef ATOM_UI_COMPAT
#include "plat.h"
#include "keys.h"
#ifdef __TICE__
#include <ti/screen.h>
static uint16_t text(const char *s, uint16_t x, uint8_t y, bool large, bool trans, uint16_t fg, uint16_t bg) {
    uint16_t e;
    os_FontSelect(large ? os_LargeFont : os_SmallFont);
    os_SetDrawFGColor(fg);
    os_SetDrawBGColor(bg);
    e = (uint16_t)(trans ? os_FontDrawTransText(s, x, y) : os_FontDrawText(s, x, y));
    os_FontSelect(os_LargeFont);
    return (uint16_t)(e - x);
}
static uint16_t width(const char *s, bool large) {
    uint16_t w;
    os_FontSelect(large ? os_LargeFont : os_SmallFont);
    w = (uint16_t)os_FontGetWidth(s);
    os_FontSelect(os_LargeFont);
    return w;
}
#else   /* host tests: the mock has only the small font */
static uint16_t text(const char *s, uint16_t x, uint8_t y, bool large, bool trans, uint16_t fg, uint16_t bg) {
    (void)large; (void)trans;
    plat_small(s, x, y, fg, bg);
    return plat_small_width(s);
}
static uint16_t width(const char *s, bool large) { (void)large; return plat_small_width(s); }
#endif

void ui_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    uint16_t *v = plat_vram(), i, j;
    for (j = y; j < y + h && j < LCD_H; j++)
        for (i = x; i < x + w && i < LCD_W; i++) v[(uint32_t)j * LCD_W + i] = c;
}

void ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    ui_fill(x, y, w, 1, c);
    ui_fill(x, (uint16_t)(y + h - 1), w, 1, c);
    ui_fill(x, y, 1, h, c);
    ui_fill((uint16_t)(x + w - 1), y, 1, h, c);
}

uint16_t ui_text(const char *s, uint16_t x, uint8_t y, uint8_t f, uint16_t fg, uint16_t bg) {
    uint16_t w = text(s, x, y, f & UI_LARGE, (f & (UI_TRANS | UI_BOLD)) != 0, fg, bg);
    if (f & UI_BOLD) { text(s, (uint16_t)(x + 1), y, f & UI_LARGE, true, fg, bg); w++; }
    return w;
}

uint16_t ui_text_width(const char *s, uint8_t f) { return (uint16_t)(width(s, f & UI_LARGE) + (f & UI_BOLD ? 1 : 0)); }

void ui_frame(const char *title, const char *view, const char *const l[5]) {
    uint8_t i = 0;
    ui_fill(0, 0, LCD_W, 30, C_SLATE);
    ui_text(title, 6, 9, 0, C_WHITE, C_SLATE);
    if (view && view[0]) ui_text(view, (uint16_t)(310 - ui_text_width(view, 0)), 9, 0, C_WHITE, C_SLATE);
    ui_fill(0, 30, LCD_W, 190, C_WHITE);
    ui_fill(0, 220, LCD_W, 1, C_BLACK);
    ui_fill(0, 221, LCD_W, 19, C_WHITE);
    while (i < 5) {
        uint8_t j = i;
        while (j + 1 < 5 && !strcmp(l[j + 1], l[i])) j++;
        if (l[i][0]) {
            uint16_t x0 = (uint16_t)(i * 64), x1 = (uint16_t)((j + 1) * 64 - 1);
            ui_box(x0, 221, (uint16_t)(x1 - x0 + 1), 19, C_BLACK);
            ui_text(l[i], (uint16_t)((x0 + x1) / 2 - ui_text_width(l[i], UI_LARGE) / 2), 222, UI_LARGE, C_BLACK,
                    C_WHITE);
        }
        i = (uint8_t)(j + 1);
    }
}

void ui_page(const char *title, const char *sub, uint16_t color, uint8_t page, uint8_t np) {
    (void)page;
    (void)np;
    ui_fill(0, 0, LCD_W, 34, color);
    ui_text(title, 6, 1, UI_LARGE | UI_BOLD, C_WHITE, color);
    ui_text(sub, 6, 20, 0, C_WHITE, color);
    ui_fill(0, 34, LCD_W, 1, C_BORDER);
    ui_fill(0, 35, LCD_W, LCD_H - 35, C_WHITE);
}

uint8_t ui_pick_at(const char *title, const char *const *items, uint8_t n, uint8_t sel) {
    (void)title;
    for (;;) {
        uint8_t i, k, y0 = (uint8_t)(219 - 20 * n);
        ui_box(0, y0, 171, (uint16_t)(20 * n + 1), C_BLACK);
        for (i = 0; i < n; i++) {
            char b[40];
            uint16_t bg = i == sel ? C_SLATE : C_WHITE;
            b[0] = (char)('1' + i);
            b[1] = ':';
            strcpy(b + 2, items[i]);
            ui_fill(1, (uint16_t)(y0 + 1 + 20 * i), 169, 20, bg);
            ui_text(b, 6, (uint8_t)(y0 + 5 + 20 * i), 0, i == sel ? C_WHITE : C_BLACK, bg);
        }
        while (!(k = plat_getcsc())) ;
        if (k == K_UP) sel = (uint8_t)((sel + n - 1) % n);
        else if (k == K_DOWN) sel = (uint8_t)((sel + 1) % n);
        else if (k == K_ENTER) return sel;
        else if (k == K_1 || k == K_2 || k == K_3 || k == K_4) {
            i = k == K_1 ? 0 : k == K_2 ? 1 : k == K_3 ? 2 : 3;
            if (i < n) return i;
        } else return 255;
    }
}
#endif
