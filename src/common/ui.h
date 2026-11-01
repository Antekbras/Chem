/* ui.h - home-screen look: 26x10 large-font grid under the OS status bar,
 * line history (input left, answer right), scroll back with 2nd+▲/▼,
 * plain decimal in small grey text on the answer row, [GRAPH] show-work
 * screen, [Y=]..[GRAPH] field bar. Never uses OS Input/Prompt. */
#ifndef CHEM_UI_H
#define CHEM_UI_H

#include "entry.h"
#include "work.h"

#define UI_COLS   26
#define UI_ROWS   10
#define UI_OUT    32
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
    uint8_t scroll;         /* rows scrolled back */
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

/* ---- drawing (ui_draw.c over plat.h) ---- */
void ui_begin(void);                    /* clear screen, status bar */
void ui_end(void);                      /* restore home screen */
void ui_draw(const screen_t *s, const entry_t *e, uint8_t rows);
/* wait for a key, blinking the cursor at its cell */
uint8_t ui_getkey(const screen_t *s, const entry_t *e, uint8_t rows);
/* full-screen show-work view; returns the key that closed it */
uint8_t ui_show_work(const char *title, const work_t *w);
/* forget what is on screen: the next ui_draw repaints every cell */
void ui_invalidate(void);
/* measured grid: origin, cell pitch, cell box size (for tests) */
void ui_grid(uint16_t *x0, uint16_t *y0, uint16_t *pw, uint16_t *ph, uint16_t *bw, uint16_t *bh);
/* glyph shown inverted at the cursor: ^ 2nd, a ALPHA, A letter lock, _ insert */
char ui_cursor_glyph(const entry_t *e);
/* bottom field bar for [Y=] [WINDOW] [ZOOM] [TRACE] [GRAPH]; sel = 0xFF none */
void ui_fields(const char *const labels[5], uint8_t sel);

#endif
