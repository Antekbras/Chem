/* app.c - echo (pure part of the app shell) */
#include <string.h>
#include "app.h"
#include "units.h"
#include "formula.h"
#include "state.h"

static bool sep(char c) { return c == ' ' || c == CH_STO || c == '+' || c == ';' || c == 0; }

static void put(char **p, char *end, const char *s, uint8_t n) {
    while (n-- && *p < end) *(*p)++ = *s++;
}

uint8_t app_echo(const char *in, uint8_t prefer, char *out, uint8_t max) {
    char *p = out, *end = out + max - 1, tmp[48];
    const char *s = in;
    while (*s) {
        uint8_t len = 0, n;
        num_t qn;
        formula_t f;
        unit_t u;
        if (sep(*s) && !(*s == '+' && s > in && !sep(s[-1]) && sep(s[1]))) {
            put(&p, end, s, 1);
            s++;
            continue;
        }
        /* a '+' right before a separator ends an ion (fe3+) */
        while (!sep(s[len]) || (s[len] == '+' && sep(s[len + 1]) && len)) len++;
        n = num_parse(s, &qn);
        if (n) {                              /* number, then maybe its unit */
            put(&p, end, s, n);
            s += n;
            len = (uint8_t)(len - n);
            if (!len) continue;
            n = unit_parse(s, &u);
            if (n == len) {
                put(&p, end, tmp, unit_str(&u, tmp));
                s += n;
                continue;
            }
        }
        if (unit_parse(s, &u) == len && len && !formula_parse(s, len, prefer, &f)) {
            put(&p, end, tmp, unit_str(&u, tmp));
        } else if (formula_parse(s, len, prefer, &f)) {
            put(&p, end, tmp, formula_str(&f, tmp));
        } else {
            put(&p, end, s, len);
        }
        s += len;
    }
    *p = 0;
    return (uint8_t)(p - out);
}

/* ---- the home-screen loop shared by every app ---- */

#define STATE_VERSION 3

typedef struct {
    entry_t e;
    screen_t s;
    uint8_t data[APP_DATA_MAX];
} app_state_t;

static app_state_t st;
static work_t work;

const entry_t *app_entry(void) { return &st.e; }
const screen_t *app_screen(void) { return &st.s; }
const work_t *app_work(void) { return &work; }

static void compute(const app_def_t *d) {
    char out[UI_OUT], small[UI_SMALL];
    out[0] = small[0] = 0;
    work_clear(&work);
    d->compute(st.e.buf, out, small, &work);
    screen_answer(&st.s, out, small);
    entry_done(&st.e);
}

void app_open(const app_def_t *d) {
    if (!state_load(d->var, &st, sizeof st, STATE_VERSION)) {
        memset(&st, 0, sizeof st);
        entry_init(&st.e, d->smart, LOCK_NUM);
        screen_clear(&st.s);
    } else if (d->data && d->data_size <= APP_DATA_MAX) {
        memcpy(d->data, st.data, d->data_size);
    }
    st.e.smart = d->smart;
    st.e.second = st.e.alpha = false;
    work_clear(&work);
    /* work is not saved: rebuild it for the answer on screen */
    if (st.s.has_out && st.e.last[0]) {
        char out[UI_OUT], small[UI_SMALL];
        uint8_t keep[APP_DATA_MAX];
        out[0] = small[0] = 0;
        /* don't let the rebuild change app data ("Ans*2" must not run twice) */
        if (d->data) memcpy(keep, d->data, d->data_size);
        d->compute(st.e.last, out, small, &work);
        if (d->data) memcpy(d->data, keep, d->data_size);
    }
    ui_begin();
    ui_draw(&st.s, &st.e, UI_ROWS);
}

bool app_key(const app_def_t *d, uint8_t k) {
    uint8_t r = entry_key(&st.e, k);
    /* any key but 2nd/ALPHA and the scroll keys returns to the bottom */
    if (r != EK_SCROLL_UP && r != EK_SCROLL_DOWN && k != K_2ND && k != K_ALPHA)
        st.s.scroll = 0;
    switch (r) {
    case EK_NEWLINE:
        screen_push(&st.s, st.e.last);
        break;
    case EK_CHANGED:
        /* CLEAR after ENTER: the finished line and its answer go to history */
        if (k == K_CLEAR && st.s.has_out) screen_push(&st.s, st.e.last);
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
        if (!st.s.n && !st.s.has_out) return false;
        screen_clear(&st.s);
        work_clear(&work);
        ui_begin();
        break;
    case EK_SCROLL_UP:
        if (st.s.scroll < ui_max_scroll(&st.s, &st.e, UI_ROWS)) st.s.scroll++;
        break;
    case EK_SCROLL_DOWN:
        if (st.s.scroll) st.s.scroll--;
        break;
    case EK_QUIT:
        return false;
    }
    ui_draw(&st.s, &st.e, UI_ROWS);
    return true;
}

void app_close(const app_def_t *d) {
    st.s.scroll = 0;
    if (d->data && d->data_size <= APP_DATA_MAX) memcpy(st.data, d->data, d->data_size);
    state_save(d->var, &st, sizeof st, STATE_VERSION);
    ui_end();
}

void app_run(const app_def_t *d) {
    app_open(d);
    while (app_key(d, ui_getkey(&st.s, &st.e, UI_ROWS))) ;
    app_close(d);
}
