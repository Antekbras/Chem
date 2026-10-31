/* app_ce.c - the home-screen loop shared by every app (calculator only) */
#ifdef __TICE__
#include <string.h>
#include "app.h"
#include "state.h"

#define STATE_VERSION 1

typedef struct {
    entry_t e;
    screen_t s;
} app_state_t;

static app_state_t st;
static work_t work;

static void compute(const app_def_t *d) {
    char out[UI_OUT], small[UI_SMALL];
    out[0] = small[0] = 0;
    work_clear(&work);
    d->compute(st.e.buf, out, small, &work);
    screen_answer(&st.s, out, small);
    entry_done(&st.e);
}

void app_run(const app_def_t *d) {
    if (!state_load(d->var, &st, sizeof st, STATE_VERSION)) {
        entry_init(&st.e, d->smart, LOCK_NUM);
        screen_clear(&st.s);
    }
    st.e.smart = d->smart;
    st.e.second = st.e.alpha = false;
    /* work is not saved: rebuild it for the line on screen */
    if (st.s.has_out && st.e.len) {
        char out[UI_OUT], small[UI_SMALL];
        d->compute(st.e.buf, out, small, &work);
    }
    ui_begin();
    ui_draw(&st.s, &st.e, UI_ROWS);
    for (;;) {
        uint8_t k = ui_getkey(&st.s, &st.e, UI_ROWS);
        switch (entry_key(&st.e, k)) {
        case EK_NEWLINE:
            screen_push(&st.s, st.e.last);
            break;
        case EK_ENTER:
            if (st.e.len) compute(d);
            break;
        case EK_GRAPH:
            if (work.n) ui_show_work(d->title, &work);
            break;
        case EK_FKEY:
            if (d->field) d->field(st.e.fkey);
            break;
        case EK_CLEAR_EMPTY:
            if (!st.s.n && !st.s.has_out) goto quit;
            screen_clear(&st.s);
            work_clear(&work);
            ui_begin();
            break;
        case EK_SCROLL_UP:
            if (st.s.scroll < HIST_N * 4) st.s.scroll++;
            break;
        case EK_SCROLL_DOWN:
            if (st.s.scroll) st.s.scroll--;
            break;
        case EK_QUIT:
            goto quit;
        case EK_NONE:
            continue;
        }
        ui_draw(&st.s, &st.e, UI_ROWS);
    }
quit:
    st.s.scroll = 0;
    state_save(d->var, &st, sizeof st, STATE_VERSION);
    ui_end();
}
#endif
