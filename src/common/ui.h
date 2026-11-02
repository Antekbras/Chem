/* ui.h - the shared screen, styled like PTABLE v2 (plan/UI_STYLE.md):
 * slate title bar, input box with a 1/A badge, answers listed under it
 * (newest first, plain decimal small grey), F-key tab bar, OPTIONS-style
 * popups and INFO-style pages. Never uses OS Input/Prompt. */
#ifndef CHEM_UI_H
#define CHEM_UI_H

#include "entry.h"
#include "work.h"

#define UI_COLS   26
#define UI_ROWS   10
#define UI_OUT    112     /* answer rows split by '\n' */
#define UI_SMALL  24
#define HIST_N    12

typedef struct {
    char in[ENTRY_MAX + 1];
    char out[UI_OUT];       /* big answer, right aligned (sci notation) */
    char small[UI_SMALL];   /* plain decimal, small grey, left of answer */
} hline_t;

typedef struct {
    uint8_t n;
    hline_t h[HIST_N];      /* oldest first */
    char out[UI_OUT];       /* answer of the current entry line */
    char small[UI_SMALL];
    bool has_out;
    uint8_t scroll;         /* history entries scrolled back */
} screen_t;

void screen_clear(screen_t *s);
/* answer for the current line (ENTER, or edit + ENTER recomputes) */
void screen_answer(screen_t *s, const char *out, const char *small);
/* current line is finished: move it and its answer into the history */
void screen_push(screen_t *s, const char *in);

/* ---- layout (pure, host-tested) ---- */
enum { ROW_IN, ROW_OUT };
typedef struct {
    uint8_t kind;           /* ROW_IN or ROW_OUT */
    const char *text;       /* not NUL-terminated for ROW_IN pieces */
    uint8_t len;
    uint8_t col;            /* start column */
    const char *small;      /* ROW_OUT only */
} srow_t;

/* Rows to show, top to bottom (at most `rows`). Returns count.
 * *cur_row / *cur_col: cursor cell (cur_row = 0xFF if scrolled off). */
uint8_t ui_layout(const screen_t *s, const entry_t *e, uint8_t rows,
                  srow_t *out, uint8_t *cur_row, uint8_t *cur_col);

/* how far 2nd+▲ can scroll back */
uint8_t ui_max_scroll(const screen_t *s, const entry_t *e, uint8_t rows);

/* ---- drawing (ui_draw.c over plat.h): PTABLE v2 style, plan/UI_STYLE.md ---- */

/* colors, BGR565 as the CE OS sets the LCD */
#ifndef RGB
#define RGB(r, g, b) (uint16_t)((((b) >> 3) << 11) | (((g) >> 2) << 5) | ((r) >> 3))
#endif
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_SLATE  RGB(106, 121, 139)   /* title bar, selected menu row */
#define C_NAVY   RGB(49, 56, 74)      /* popup / page headers, mode badge */
#define C_BORDER RGB(82, 80, 82)
#define C_SHADOW RGB(123, 121, 123)
#define C_BAND   RGB(246, 242, 246)   /* alternate rows */
#define C_SELECT RGB(205, 226, 255)   /* current answer, selected list row */
#define C_GREY   RGB(110, 110, 110)   /* hints, labels */

/* text flags (same bits as plat.h) */
#define UI_LARGE 1
#define UI_TRANS 2
#define UI_BOLD  4

#define UI_TAB_Y  220               /* tab bar line; tabs below it */
#define UI_BODY_Y 30                /* first body row under the title bar */

/* app identity for the frame: title bar text, hint under the input box,
 * view name at the right of the title bar, tab labels for [Y=]..[GRAPH]
 * ("WORK" = show work, "QUIT" = quit, "" = no tab, else the app's field) */
void ui_setup(const char *heading, const char *hint, const char *const labels[5]);
void ui_set_view(const char *view);
const char *const *ui_labels(void);

void ui_begin(void);                    /* draw the whole frame */
void ui_end(void);                      /* give the screen back to the OS */
/* draw the input box and, when they changed, the answers */
void ui_draw(const screen_t *s, const entry_t *e, uint8_t rows);
/* wait for a key, blinking the caret */
uint8_t ui_getkey(const screen_t *s, const entry_t *e, uint8_t rows);
/* the next ui_draw repaints everything (after a popup or page) */
void ui_invalidate(void);
/* tests: the full main screen, with the caret on or off */
void ui_redraw_all(const screen_t *s, const entry_t *e, bool caret);
/* mode badge text: 1 number lock, A letter lock, a ALPHA, 2 after 2nd */
char ui_cursor_glyph(const entry_t *e);

/* frame pieces */
void ui_title(void);
/* title bar + white body + tab bar for an app screen (home heading kept) */
void ui_frame(const char *title, const char *view, const char *const labels[5]);
void ui_tabs(const char *const labels[5], uint8_t sel);   /* sel = 0xFF none */
#define ui_fields ui_tabs

/* primitives */
void ui_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
void ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);   /* 1 px outline */
uint16_t ui_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg);
uint16_t ui_text_width(const char *s, uint8_t flags);
uint8_t ui_text_height(uint8_t flags);
/* popup panel: white, border, drop shadow */
void ui_panel(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/* pages (show work, info): header in `color`, name + subtitle, page dots */
void ui_page(const char *name, const char *sub, uint16_t color, uint8_t page, uint8_t npages);
/* page row i (21 px from y=39, banded): label small grey + value large,
 * or (label NULL) a plain large line */
void ui_row(uint8_t i, const char *label, const char *value);
#define UI_PAGE_ROWS 9

/* one screen of show work starting at line `top` (ui_show_work's frame) */
void ui_work_page(const char *title, const work_t *w, uint8_t top);
/* show work as a page; ▲▼ scroll, ◄► page; returns the key that closed it */
uint8_t ui_show_work(const char *title, const work_t *w);
/* popup list like PTABLE's OPTIONS (or Find for long lists): ▲▼ move,
 * ENTER or 1-9/0 picks, CLEAR cancels. Returns the index or 0xFF. */
uint8_t ui_pick(const char *title, const char *const *items, uint8_t n);
uint8_t ui_pick_at(const char *title, const char *const *items, uint8_t n, uint8_t sel);

/* legacy grid (old home-screen cells), kept so older app code builds */
void ui_grid(uint16_t *x0, uint16_t *y0, uint16_t *pw, uint16_t *ph, uint16_t *bw, uint16_t *bh);

#endif
