/* history and home-screen layout */
#include "../src/common/ui.h"
#include "test.h"

static char rowbuf[UI_COLS + 1];
static const char *row(const srow_t *r) {
    memset(rowbuf, ' ', UI_COLS);
    rowbuf[UI_COLS] = 0;
    memcpy(rowbuf + r->col, r->text, r->len);
    return rowbuf;
}

int main(void) {
    static screen_t s;
    entry_t e;
    srow_t r[UI_ROWS];
    uint8_t n, cr, cc, i;
    screen_clear(&s);
    entry_init(&e, SMART_ALL, LOCK_NUM);

    /* empty screen: cursor top-left */
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(n == 1 && cr == 0 && cc == 0, "empty n=%d", n);

    /* entry + answer: input left, answer right-aligned, small decimal kept */
    entry_set(&e, "2.45cm2");
    screen_answer(&s, "2.45\x1B" "2 mm\x12", "245");
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(n == 2, "rows %d", n);
    CHECK_STR(row(&r[0]), "2.45cm2                   ");
    CHECK(r[1].kind == ROW_OUT && r[1].col == UI_COLS - 10 && !strcmp(r[1].small, "245"), "answer row");
    CHECK(cr == 0 && cc == 7, "cursor %d,%d", cr, cc);

    /* push to history; long input wraps at 26 columns */
    screen_push(&s, e.buf);
    entry_set(&e, "100.0g c7h6o+c2h4o\x1C" "c9h8o+h2o\x1C" "100.0g c9h8o");
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(n == 4, "wrap rows %d", n);
    CHECK(r[2].len == UI_COLS && r[3].len == e.len - UI_COLS, "wrap split");
    CHECK(cr == 3 && cc == e.len - UI_COLS, "cursor on wrapped row");

    /* cursor after exactly 26 chars gets its own cell on the next row */
    entry_set(&e, "abcdefghijklmnopqrstuvwxyz");
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(cr == n - 1 && cc == 0 && r[n - 1].len == 0, "cursor wraps");

    /* history keeps the last HIST_N lines; window is bottom-anchored */
    for (i = 0; i < HIST_N + 4; i++) {
        char in[8];
        in[0] = (char)('0' + i); in[1] = 0;
        screen_answer(&s, in, "");
        screen_push(&s, in);
    }
    CHECK(s.n == HIST_N && s.h[HIST_N - 1].in[0] == (char)('0' + HIST_N + 3), "history ring");
    entry_set(&e, "7");
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(n == UI_ROWS && cr == UI_ROWS - 1, "bottom anchored");
    /* scroll back moves the window up and hides the cursor */
    s.scroll = 4;
    n = ui_layout(&s, &e, UI_ROWS, r, &cr, &cc);
    CHECK(cr == 0xFF, "scrolled cursor hidden");
    /* field bar takes a row */
    s.scroll = 0;
    n = ui_layout(&s, &e, UI_ROWS - 1, r, &cr, &cc);
    CHECK(n == UI_ROWS - 1 && cr == UI_ROWS - 2, "fields row");

    /* show-work lines clip to the screen width */
    {
        work_t w;
        work_clear(&w);
        work_add(&w, "n=m/M=25/74.55", "=", "0.335353535353535353535", NULL);
        CHECK(strlen(w.line[0]) == WORK_COLS && w.n == 1, "work clip");
    }
    TEST_END();
}
