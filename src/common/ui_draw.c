/* ui_draw.c - the shared screen in PTABLE v2 style (plan/UI_STYLE.md).
 *
 * Frame: slate title bar (y 0-29), white body, F-key tab bar (y 220-239).
 * Main screen: input box with a 1/A badge, a hint line, then the answers,
 * newest first. A keypress redraws only the input box; the answer list is
 * redrawn when its content changes. Every region is filled before it is
 * drawn, so nothing from an earlier frame can be left behind. */
#include <string.h>
#include "ui.h"
#include "plat.h"
#include "keys.h"

/* ---- primitives ---- */

void ui_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    uint16_t *v = plat_vram();
    uint16_t i, j;
    if (x >= LCD_W || y >= LCD_H) return;
    if (x + w > LCD_W) w = (uint16_t)(LCD_W - x);
    if (y + h > LCD_H) h = (uint16_t)(LCD_H - y);
    for (j = 0; j < h; j++) {
        uint16_t *p = v + (uint32_t)(y + j) * LCD_W + x;
        for (i = 0; i < w; i++) *p++ = c;
    }
}

void ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    if (!w || !h) return;
    ui_fill(x, y, w, 1, c);
    ui_fill(x, (uint16_t)(y + h - 1), w, 1, c);
    ui_fill(x, y, 1, h, c);
    ui_fill((uint16_t)(x + w - 1), y, 1, h, c);
}

static void invert(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t *v = plat_vram();
    uint16_t i, j;
    if (x >= LCD_W || y >= LCD_H) return;
    if (x + w > LCD_W) w = (uint16_t)(LCD_W - x);
    if (y + h > LCD_H) h = (uint16_t)(LCD_H - y);
    for (j = 0; j < h; j++) {
        uint16_t *p = v + (uint32_t)(y + j) * LCD_W + x;
        for (i = 0; i < w; i++, p++) *p ^= 0xFFFF;
    }
}

void ui_panel(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    ui_fill((uint16_t)(x + 3), (uint16_t)(y + h), w, 3, C_SHADOW);
    ui_fill((uint16_t)(x + w), (uint16_t)(y + 3), 3, h, C_SHADOW);
    ui_fill(x, y, w, h, C_WHITE);
    ui_box(x, y, w, h, C_BORDER);
}

uint16_t ui_text_width(const char *s, uint8_t flags) {
    uint16_t w = plat_text_width(s, (uint8_t)(flags & UI_LARGE));
    return (uint16_t)(w + ((flags & UI_BOLD) && w ? 1 : 0));
}

uint8_t ui_text_height(uint8_t flags) { return plat_text_height((uint8_t)(flags & UI_LARGE)); }

uint16_t ui_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg) {
    uint8_t f = (uint8_t)(flags & (UI_LARGE | UI_TRANS));
    uint16_t w;
    if (!s[0]) return 0;
    if (flags & UI_BOLD) {
        if (!(flags & UI_TRANS)) ui_fill(x, y, ui_text_width(s, flags), ui_text_height(flags), bg);
        plat_text(s, x, y, (uint8_t)(f | PLAT_TRANS), fg, bg);
        w = plat_text(s, (uint16_t)(x + 1), y, (uint8_t)(f | PLAT_TRANS), fg, bg);
        return (uint16_t)(w + 1);
    }
    return plat_text(s, x, y, f, fg, bg);
}

/* the longest prefix of s (at most n chars) that fits in maxw pixels */
static uint8_t fit(const char *s, uint8_t n, uint8_t flags, uint16_t maxw, char *buf) {
    uint8_t k = 0;
    while (k < n && s[k]) {
        buf[k] = s[k];
        buf[k + 1] = 0;
        if (ui_text_width(buf, flags) > maxw) break;
        k++;
    }
    buf[k] = 0;
    return k;
}

/* text clipped to maxw */
static uint16_t text_clip(const char *s, uint8_t n, uint16_t x, uint8_t y, uint8_t flags,
                          uint16_t fg, uint16_t bg, uint16_t maxw) {
    char buf[UI_OUT + 1];
    if (n > UI_OUT) n = UI_OUT;
    fit(s, n, flags, maxw, buf);
    return ui_text(buf, x, y, flags, fg, bg);
}

/* ---- frame ---- */

static const char *heading = "";
static const char *hint = "";
static const char *view = "";
static const char *const *labels;
static uint8_t LH = 16, SH = 12;          /* font heights, read in ui_begin */

/* main-screen geometry (from the font heights) */
#define BOX_X 4
#define BOX_W 312
#define BOX_Y 34
#define BOX_H ((uint16_t)(LH + 6))
#define TEXT_X 9
#define BADGE_W 21
#define BADGE_X ((uint16_t)(BOX_X + BOX_W - BADGE_W - 3))
#define TEXT_W ((uint16_t)(BADGE_X - TEXT_X - 4))
#define HINT_Y ((uint16_t)(BOX_Y + BOX_H + 3))
#define SEP_Y ((uint16_t)(HINT_Y + SH + 3))
#define LIST_Y ((uint16_t)(SEP_Y + 2))
#define LIST_END UI_TAB_Y

void ui_setup(const char *h, const char *t, const char *const l[5]) {
    heading = h ? h : "";
    hint = t ? t : "";
    labels = l;
}

void ui_set_view(const char *v) { view = v ? v : ""; }
const char *const *ui_labels(void) { return labels; }

void ui_title(void) {
    uint8_t b = plat_battery(), i;
    uint8_t ty = (uint8_t)((STATUS_H - SH) / 2);
    ui_fill(0, 0, LCD_W, STATUS_H, C_SLATE);
    text_clip(heading, 40, 6, ty, 0, C_WHITE, C_SLATE, 200);
    if (view[0]) {
        uint16_t w = ui_text_width(view, 0);
        if (w > 90) w = 90;
        text_clip(view, 30, (uint16_t)(296 - w), ty, 0, C_WHITE, C_SLATE, 90);
    }
    /* battery: cap, outline, up to 4 green bars from the bottom */
    ui_fill(305, 6, 4, 2, C_WHITE);
    ui_box(302, 8, 10, 16, C_WHITE);
    if (b > 4) b = 4;
    for (i = 0; i < b; i++)
        ui_fill(304, (uint16_t)(20 - i * 3), 6, 2, RGB(40, 200, 40));
}

void ui_tabs(const char *const l[5], uint8_t sel) {
    uint8_t i = 0;
    ui_fill(0, UI_TAB_Y, LCD_W, 1, C_BLACK);
    ui_fill(0, UI_TAB_Y + 1, LCD_W, LCD_H - UI_TAB_Y - 1, C_WHITE);
    if (!l) return;
    while (i < 5) {
        uint8_t j = i;
        const char *t = l[i] ? l[i] : "";
        while (j + 1 < 5 && l[j + 1] && t[0] && !strcmp(l[j + 1], t)) j++;
        if (t[0]) {
            uint16_t x = (uint16_t)(i * 64), w = (uint16_t)((j - i + 1) * 64);
            uint16_t bg = (i <= sel && sel <= j) ? C_SLATE : C_WHITE;
            uint16_t fg = bg == C_WHITE ? C_BLACK : C_WHITE;
            uint16_t tw = ui_text_width(t, UI_LARGE);
            uint8_t ty = (uint8_t)(UI_TAB_Y + 1 + (LCD_H - UI_TAB_Y - 1 - LH) / 2);
            ui_fill((uint16_t)(x + 1), UI_TAB_Y + 2, (uint16_t)(w - 2), LCD_H - UI_TAB_Y - 2, bg);
            ui_box(x, UI_TAB_Y + 1, w, LCD_H - UI_TAB_Y - 1, C_BLACK);
            /* rounded top corners */
            ui_fill(x, UI_TAB_Y + 1, 1, 1, C_WHITE);
            ui_fill((uint16_t)(x + w - 1), UI_TAB_Y + 1, 1, 1, C_WHITE);
            if (tw > w - 4) tw = (uint16_t)(w - 4);
            text_clip(t, 12, (uint16_t)(x + (w - tw) / 2), ty, UI_LARGE, fg, bg, (uint16_t)(w - 4));
        }
        i = (uint8_t)(j + 1);
    }
}

/* ---- main screen ---- */

static bool dirty_frame = true, dirty_list = true;
static uint16_t list_sum;
static uint8_t view_start;                 /* first input char shown */
static uint16_t caret_x, caret_w;          /* caret box, for blinking */

void ui_invalidate(void) { dirty_frame = dirty_list = true; }

/* a whole frame for an app's own screen (table, forms): title bar with
 * `title` and `view`, white body, tab bar. The home screen's own heading
 * and view are left as they were. */
void ui_frame(const char *title, const char *v, const char *const l[5]) {
    const char *h0 = heading, *v0 = view;
    LH = plat_text_height(PLAT_LARGE);
    SH = plat_text_height(0);
    heading = title ? title : "";
    view = v ? v : "";
    ui_title();
    heading = h0;
    view = v0;
    ui_fill(0, UI_BODY_Y, LCD_W, UI_TAB_Y - UI_BODY_Y, C_WHITE);
    ui_tabs(l, 0xFF);
}

void ui_begin(void) {
    LH = plat_text_height(PLAT_LARGE);
    SH = plat_text_height(0);
    ui_invalidate();
}

void ui_end(void) {
    plat_home_reset();
}

char ui_cursor_glyph(const entry_t *e) {
    if (e->second) return '2';
    if (e->alpha) return 'a';
    if (e->lock == LOCK_LET) return 'A';
    return '1';
}

static void draw_box(const entry_t *e) {
    char buf[ENTRY_MAX + 1], g[2];
    uint8_t k, n;
    uint16_t w;
    /* keep the cursor in view */
    if (e->cur < view_start) view_start = e->cur;
    if (view_start > e->len) view_start = 0;
    for (;;) {
        n = (uint8_t)(e->cur - view_start + (e->cur < e->len ? 1 : 0));
        memcpy(buf, e->buf + view_start, n);
        buf[n] = 0;
        w = ui_text_width(buf, UI_LARGE) + (e->cur < e->len ? 0 : 8);
        if (w <= TEXT_W || view_start >= e->cur) break;
        view_start++;
    }
    ui_box(BOX_X, BOX_Y, BOX_W, BOX_H, C_BORDER);
    ui_fill(BOX_X + 1, BOX_Y + 1, BOX_W - 2, (uint16_t)(BOX_H - 2), C_WHITE);
    k = fit(e->buf + view_start, (uint8_t)(e->len - view_start), UI_LARGE, TEXT_W, buf);
    (void)k;
    ui_text(buf, TEXT_X, BOX_Y + 3, UI_LARGE, C_BLACK, C_WHITE);
    /* caret box: the character under the cursor (overwrite) or a bar (insert) */
    n = (uint8_t)(e->cur - view_start);
    memcpy(buf, e->buf + view_start, n);
    buf[n] = 0;
    caret_x = (uint16_t)(TEXT_X + ui_text_width(buf, UI_LARGE));
    if (e->ins) caret_w = 2;
    else if (e->cur < e->len) {
        g[0] = e->buf[e->cur]; g[1] = 0;
        caret_w = ui_text_width(g, UI_LARGE);
        if (!caret_w) caret_w = 8;
    } else caret_w = 8;
    if (caret_x + caret_w > BADGE_X - 2) caret_w = (uint16_t)(BADGE_X - 2 > caret_x ? BADGE_X - 2 - caret_x : 1);
    /* mode badge */
    g[0] = ui_cursor_glyph(e); g[1] = 0;
    ui_fill(BADGE_X, BOX_Y + 2, BADGE_W, (uint16_t)(BOX_H - 4), C_NAVY);
    w = ui_text_width(g, UI_LARGE);
    ui_text(g, (uint16_t)(BADGE_X + (BADGE_W - w) / 2), BOX_Y + 3, UI_LARGE, C_WHITE, C_NAVY);
}

static void caret(void) {
    invert(caret_x, BOX_Y + 3, caret_w, LH);
}

/* one answer block at y: input (small grey), answer lines (large, right),
 * alt (small grey, left of the first answer line). Returns its height. */
static uint16_t item(uint16_t y, const char *in, const char *out, const char *alt, uint16_t bg, bool draw) {
    uint16_t h = 2, y0 = y;
    const char *p = out;
    if (in[0]) h = (uint16_t)(h + SH + 1);
    if (out[0]) {
        h = (uint16_t)(h + LH + 1);
        while ((p = strchr(p, '\n')) != NULL) { p++; h = (uint16_t)(h + LH + 1); }
    }
    h = (uint16_t)(h + 2);
    if (!draw) return h;
    ui_fill(0, y, LCD_W, (uint16_t)(y + h > LIST_END ? LIST_END - y : h), bg);
    y = (uint16_t)(y + 2);
    if (in[0] && y + SH <= LIST_END) {
        text_clip(in, ENTRY_MAX, 8, (uint8_t)y, 0, C_GREY, bg, 304);
        y = (uint16_t)(y + SH + 1);
    }
    p = out;
    while (p && *p && y + LH <= LIST_END) {
        const char *nl = strchr(p, '\n');
        uint8_t n = (uint8_t)(nl ? nl - p : (long)strlen(p));
        char buf[UI_OUT + 1];
        uint16_t w;
        fit(p, n, UI_LARGE, 304, buf);
        w = ui_text_width(buf, UI_LARGE);
        ui_text(buf, (uint16_t)(312 - w), (uint8_t)y, UI_LARGE, C_BLACK, bg);
        if (p == out && alt && alt[0]) {
            uint16_t aw = ui_text_width(alt, 0);
            if (aw + 16 <= 312 - w)
                ui_text(alt, 8, (uint8_t)(y + LH - SH), 0, C_GREY, bg);
        }
        y = (uint16_t)(y + LH + 1);
        p = nl ? nl + 1 : NULL;
    }
    (void)y0;
    return h;
}

static uint16_t sum_str(uint16_t a, const char *s) {
    while (*s) a = (uint16_t)((a << 5) + (a >> 11) + (uint8_t)*s++);
    return (uint16_t)(a * 31 + 7);
}

static uint16_t list_hash(const screen_t *s, const entry_t *e) {
    uint16_t a = (uint16_t)(s->n * 131 + s->scroll * 7 + s->has_out);
    uint8_t i;
    if (s->has_out) { a = sum_str(a, e->last); a = sum_str(a, s->out); a = sum_str(a, s->small); }
    for (i = 0; i < s->n; i++) {
        a = sum_str(a, s->h[i].in);
        a = sum_str(a, s->h[i].out);
        a = sum_str(a, s->h[i].small);
    }
    return a;
}

static void draw_list(const screen_t *s, const entry_t *e) {
    uint16_t y = LIST_Y;
    uint8_t skip = s->scroll, band = 0;
    int i;
    if (s->has_out) {
        if (skip) skip--;
        else y = (uint16_t)(y + item(y, e->last, s->out, s->small, C_SELECT, true));
    }
    for (i = s->n - 1; i >= 0 && y < LIST_END; i--) {
        if (skip) { skip--; continue; }
        y = (uint16_t)(y + item(y, s->h[i].in, s->h[i].out, s->h[i].small, band ? C_BAND : C_WHITE, true));
        band ^= 1;
    }
    if (y < LIST_END) ui_fill(0, y, LCD_W, (uint16_t)(LIST_END - y), C_WHITE);
}

void ui_draw(const screen_t *s, const entry_t *e, uint8_t rows) {
    uint16_t sum = list_hash(s, e);
    (void)rows;
    if (dirty_frame) {
        ui_title();
        ui_fill(0, STATUS_H, LCD_W, (uint16_t)(UI_TAB_Y - STATUS_H), C_WHITE);
        if (hint[0]) text_clip(hint, 60, 8, (uint8_t)HINT_Y, 0, C_GREY, C_WHITE, 304);
        ui_fill(0, SEP_Y, LCD_W, 1, C_BORDER);
        ui_tabs(labels, 0xFF);
        dirty_frame = false;
        dirty_list = true;
    }
    draw_box(e);
    if (dirty_list || sum != list_sum) {
        draw_list(s, e);
        list_sum = sum;
        dirty_list = false;
    }
}

void ui_redraw_all(const screen_t *s, const entry_t *e, bool on) {
    ui_invalidate();
    ui_draw(s, e, UI_ROWS);
    if (on) caret();
}

uint8_t ui_getkey(const screen_t *s, const entry_t *e, uint8_t rows) {
    uint8_t k;
    bool on = true;
    uint32_t t = plat_ticks();
    (void)s; (void)e; (void)rows;
    caret();
    while (!(k = plat_getcsc())) {
        if (plat_ticks() - t > PLAT_TICKS_PER_SEC / 2) {
            on = !on;
            caret();
            t = plat_ticks();
        }
    }
    if (on) caret();
    return k;
}

/* ---- pages ---- */

static bool dark(uint16_t c) {
    /* BGR565: r = low 5 bits, g = middle 6, b = high 5 */
    uint16_t r = (uint16_t)((c & 31) << 3), g = (uint16_t)(((c >> 5) & 63) << 2), b = (uint16_t)((c >> 11) << 3);
    return r * 3 + g * 6 + b < 128 * 10;
}

void ui_page(const char *name, const char *sub, uint16_t color, uint8_t page, uint8_t npages) {
    uint16_t fg = dark(color) ? C_WHITE : C_BLACK;
    uint8_t i;
    ui_fill(0, 0, LCD_W, 34, color);
    text_clip(name, 40, 6, 2, UI_LARGE | UI_BOLD, fg, color, 300);
    if (sub && sub[0]) text_clip(sub, 60, 6, (uint8_t)(4 + LH), 0, fg, color, 260);
    if (npages > 1) {
        if (npages > 8) npages = 8;
        for (i = 0; i < npages; i++) {
            uint16_t x = (uint16_t)(314 - (npages - i) * 6);
            if (i == page) ui_fill(x, 26, 4, 4, fg);
            else ui_box(x, 26, 4, 4, fg);
        }
    }
    ui_fill(0, 34, LCD_W, 1, C_BORDER);
    ui_fill(0, 35, LCD_W, LCD_H - 35, C_WHITE);
}

void ui_row(uint8_t i, const char *label, const char *value) {
    uint16_t y = (uint16_t)(39 + i * 21), bg = (i & 1) ? C_BAND : C_WHITE;
    if (y + 21 > LCD_H) return;
    ui_fill(0, y, LCD_W, 21, bg);
    if (label) {
        text_clip(label, 40, 12, (uint8_t)(y + (21 - SH) / 2), 0, C_GREY, bg, 106);
        if (value) text_clip(value, UI_OUT, 122, (uint8_t)(y + (21 - LH) / 2), UI_LARGE, C_BLACK, bg, 194);
    } else if (value) {
        text_clip(value, UI_OUT, 8, (uint8_t)(y + (21 - LH) / 2), UI_LARGE, C_BLACK, bg, 306);
    }
}

void ui_work_page(const char *title, const work_t *w, uint8_t top) {
    uint8_t i, np = (uint8_t)((w->n + UI_PAGE_ROWS - 1) / UI_PAGE_ROWS);
    LH = plat_text_height(PLAT_LARGE);
    SH = plat_text_height(0);
    ui_page(title, "Show work - arrows scroll, CLEAR back", C_NAVY,
            (uint8_t)((top + UI_PAGE_ROWS / 2) / UI_PAGE_ROWS), np);
    for (i = 0; i < UI_PAGE_ROWS; i++) {
        uint8_t j = (uint8_t)(top + i);
        ui_row(i, NULL, j < w->n ? w->line[j] : "");
    }
}

uint8_t ui_show_work(const char *title, const work_t *w) {
    uint8_t top = 0, k;
    ui_invalidate();
    for (;;) {
        ui_work_page(title, w, top);
        while (!(k = plat_getcsc())) ;
        if (k == K_UP) { if (top) top--; }
        else if (k == K_DOWN) { if (top + UI_PAGE_ROWS < w->n) top++; }
        else if (k == K_LEFT) top = (uint8_t)(top > UI_PAGE_ROWS ? top - UI_PAGE_ROWS : 0);
        else if (k == K_RIGHT) {
            if (w->n > UI_PAGE_ROWS) {
                top = (uint8_t)(top + UI_PAGE_ROWS);
                if (top + UI_PAGE_ROWS > w->n) top = (uint8_t)(w->n - UI_PAGE_ROWS);
            }
        }
        else return k;
    }
}

/* ---- popups ---- */

static uint8_t digit(uint8_t k) {
    static const uint8_t dig[10] = {K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_0};
    uint8_t i;
    for (i = 0; i < 10; i++) if (k == dig[i]) return i;
    return 0xFF;
}

static void label(char *row, uint8_t j, const char *item) {
    row[0] = j < 9 ? (char)('1' + j) : j == 9 ? '0' : ' ';
    row[1] = j < 10 ? ':' : ' ';
    strncpy(row + 2, item, 40);
    row[42] = 0;
}

uint8_t ui_pick_at(const char *title, const char *const *items, uint8_t n, uint8_t sel) {
    char row[44];
    uint8_t i, k, top = 0;
    bool menu = n <= 8;                    /* OPTIONS menu; else a Find-style list */
    uint16_t x0, y0, w, rh, vis;
    if (!n) return 0xFF;
    LH = plat_text_height(PLAT_LARGE);
    SH = plat_text_height(0);
    if (sel >= n) sel = 0;
    if (menu) {
        w = 170;
        for (i = 0; i < n; i++) {
            uint16_t tw;
            label(row, i, items[i]);
            tw = (uint16_t)(ui_text_width(row, 0) + 16);
            if (tw > w) w = tw;
        }
        if (w > 316) w = 316;
        rh = 20; vis = n;
        x0 = 0; y0 = (uint16_t)(UI_TAB_Y - n * rh - 1);
        ui_fill(x0, y0, w, (uint16_t)(n * rh + 1), C_WHITE);
        ui_box(x0, y0, w, (uint16_t)(n * rh + 1), C_BLACK);
    } else {
        x0 = 28; y0 = 26; w = 264; rh = 18;
        vis = (uint16_t)((214 - 52) / rh);
        ui_panel(x0, y0, w, 188);
        ui_fill((uint16_t)(x0 + 1), (uint16_t)(y0 + 1), (uint16_t)(w - 2), 20, C_NAVY);
        text_clip(title, 30, (uint16_t)(x0 + 8), (uint8_t)(y0 + 2), UI_LARGE, C_WHITE, C_NAVY, 150);
        {
            uint16_t cw = ui_text_width("CLEAR: cancel", 0);
            ui_text("CLEAR: cancel", (uint16_t)(x0 + w - 6 - cw), (uint8_t)(y0 + 4), 0, C_WHITE, C_NAVY);
        }
    }
    for (;;) {
        uint16_t ys = menu ? (uint16_t)(y0 + 1) : (uint16_t)(y0 + 26);
        if (sel < top) top = sel;
        if (sel >= top + vis) top = (uint8_t)(sel - vis + 1);
        for (i = 0; i < vis; i++) {
            uint8_t j = (uint8_t)(top + i);
            uint16_t y = (uint16_t)(ys + i * rh);
            uint16_t bg = j == sel ? (menu ? C_SLATE : C_SELECT) : C_WHITE;
            uint16_t fg = (j == sel && menu) ? C_WHITE : C_BLACK;
            ui_fill((uint16_t)(x0 + 1), y, (uint16_t)(w - (menu ? 2 : 8)), rh, bg);
            if (j < n) {
                label(row, j, items[j]);
                text_clip(row, 42, (uint16_t)(x0 + 6), (uint8_t)(y + (rh - (menu ? SH : LH)) / 2),
                          menu ? 0 : UI_LARGE, fg, bg, (uint16_t)(w - 16));
            }
        }
        if (!menu && n > vis) {            /* scrollbar */
            uint16_t bh = (uint16_t)(vis * rh), th = (uint16_t)(bh * vis / n), ty;
            if (th < 6) th = 6;
            ty = (uint16_t)(ys + (bh - th) * top / (n - vis));
            ui_fill((uint16_t)(x0 + w - 6), ys, 4, bh, C_BAND);
            ui_fill((uint16_t)(x0 + w - 6), ty, 4, th, C_SHADOW);
        }
        while (!(k = plat_getcsc())) ;
        if (k == K_UP) sel = sel ? (uint8_t)(sel - 1) : (uint8_t)(n - 1);
        else if (k == K_DOWN) sel = (uint8_t)(sel + 1 < n ? sel + 1 : 0);
        else if (k == K_ENTER) { ui_invalidate(); return sel; }
        else {
            uint8_t d = digit(k);
            ui_invalidate();
            return d < n ? d : 0xFF;
        }
    }
}

uint8_t ui_pick(const char *title, const char *const *items, uint8_t n) {
    return ui_pick_at(title, items, n, 0);
}

void ui_grid(uint16_t *x0, uint16_t *y0, uint16_t *pw, uint16_t *ph, uint16_t *bw, uint16_t *bh) {
    *x0 = 0; *y0 = STATUS_H; *pw = 12; *ph = 19; *bw = 12; *bh = 19;
}
