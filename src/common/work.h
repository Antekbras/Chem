/* work.h - show-work lines collected while computing ([GRAPH] screen). */
#ifndef CHEM_WORK_H
#define CHEM_WORK_H

#include <stdint.h>

#define WORK_LINES 24
#define WORK_COLS  26

typedef struct {
    uint8_t n;
    char line[WORK_LINES][WORK_COLS + 1];
} work_t;

void work_clear(work_t *w);
/* append the concatenation of up to 4 strings (NULLs skipped), clipped to 26 chars */
void work_add(work_t *w, const char *a, const char *b, const char *c, const char *d);

#endif
