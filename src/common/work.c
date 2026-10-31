#include <string.h>
#include "work.h"

void work_clear(work_t *w) { if (w) w->n = 0; }

void work_add(work_t *w, const char *a, const char *b, const char *c, const char *d) {
    const char *parts[4];
    char *p;
    uint8_t i, left = WORK_COLS;
    if (!w || w->n >= WORK_LINES) return;
    parts[0] = a; parts[1] = b; parts[2] = c; parts[3] = d;
    p = w->line[w->n++];
    for (i = 0; i < 4; i++) {
        const char *s = parts[i];
        if (!s) continue;
        while (*s && left) { *p++ = *s++; left--; }
    }
    *p = 0;
}
