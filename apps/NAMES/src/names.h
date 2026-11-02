/* names.h - NAMES engine: formula -> name, name -> formula, info line. */
#ifndef CHEM_NAMES_H
#define CHEM_NAMES_H

#include "work.h"

/* `in` is the typed line (letters lowercase). out = "answer\ninfo",
 * small = the other reading of an ambiguous formula ("Co: cobalt"). */
void names_compute(const char *in, char *out, char *small, work_t *w);

#endif
