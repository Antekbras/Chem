/* ui.c - history and layout (pure) */
#include <string.h>
#include "ui.h"

static void copy(char *d, const char *s, size_t n) {
    strncpy(d, s, n - 1);
    d[n - 1] = 0;
}

void screen_clear(screen_t *s) { memset(s, 0, sizeof *s); }

void screen_answer(screen_t *s, const char *out, const char *small) {
    copy(s->out, out ? out : "", UI_OUT);
    copy(s->small, small ? small : "", UI_SMALL);
    s->has_out = true;
    s->scroll = 0;
}

void screen_push(screen_t *s, const char *in) {
    hline_t *h;
    if (s->n == HIST_N) {
        memmove(&s->h[0], &s->h[1], sizeof(hline_t) * (HIST_N - 1));
        s->n--;
    }
    h = &s->h[s->n++];
    copy(h->in, in, sizeof h->in);
    copy(h->out, s->has_out ? s->out : "", UI_OUT);
    copy(h->small, s->has_out ? s->small : "", UI_SMALL);
    s->has_out = false;
    s->out[0] = s->small[0] = 0;
    s->scroll = 0;
}

#define MAXROWS (HIST_N * 6 + 8)

static uint8_t add_in(srow_t *r, uint8_t n, const char *t, uint8_t len) {
    uint8_t i = 0;
    do {
        uint8_t k = (uint8_t)(len - i > UI_COLS ? UI_COLS : len - i);
        if (n >= MAXROWS) return n;
        r[n].kind = ROW_IN; r[n].text = t + i; r[n].len = k; r[n].col = 0; r[n].small = 0;
        n++;
        i = (uint8_t)(i + k);
    } while (i < len);
    return n;
}

static uint8_t add_out(srow_t *r, uint8_t n, const char *out, const char *small) {
    uint8_t len = (uint8_t)strlen(out);
    if (n >= MAXROWS) return n;
    if (len > UI_COLS) len = UI_COLS;
    r[n].kind = ROW_OUT; r[n].text = out; r[n].len = len;
    r[n].col = (uint8_t)(UI_COLS - len); r[n].small = small;
    return (uint8_t)(n + 1);
}

static uint8_t all_rows(const screen_t *s, const entry_t *e, srow_t *all,
                        uint8_t *crow, uint8_t *ccol) {
    uint8_t n = 0, i, first_cur;
    for (i = 0; i < s->n; i++) {
        n = add_in(all, n, s->h[i].in, (uint8_t)strlen(s->h[i].in));
        if (s->h[i].out[0]) n = add_out(all, n, s->h[i].out, s->h[i].small);
    }
    first_cur = n;
    /* the cursor may sit just past the last character: give it a cell */
    n = add_in(all, n, e->buf, (uint8_t)(e->len + (e->cur == e->len && e->len && e->len % UI_COLS == 0)));
    if (e->cur == e->len && e->len && e->len % UI_COLS == 0) all[n - 1].len--;
    *crow = (uint8_t)(first_cur + e->cur / UI_COLS);
    *ccol = (uint8_t)(e->cur % UI_COLS);
    if (s->has_out) n = add_out(all, n, s->out, s->small);
    return n;
}

uint8_t ui_max_scroll(const screen_t *s, const entry_t *e, uint8_t rows) {
    srow_t all[MAXROWS];
    uint8_t crow, ccol, n = all_rows(s, e, all, &crow, &ccol);
    return (uint8_t)(n > rows ? n - rows : 0);
}

uint8_t ui_layout(const screen_t *s, const entry_t *e, uint8_t rows,
                  srow_t *out, uint8_t *cur_row, uint8_t *cur_col) {
    srow_t all[MAXROWS];
    uint8_t n, i, crow, ccol, start, cnt;
    n = all_rows(s, e, all, &crow, &ccol);
    /* bottom-anchored window, moved up by scroll */
    cnt = n < rows ? n : rows;
    start = (uint8_t)(n - cnt);
    start = (uint8_t)(start > s->scroll ? start - s->scroll : 0);
    for (i = 0; i < cnt; i++) out[i] = all[start + i];
    *cur_row = (crow >= start && crow < start + cnt) ? (uint8_t)(crow - start) : 0xFF;
    *cur_col = ccol;
    return cnt;
}
