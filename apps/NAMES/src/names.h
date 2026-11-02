/* names.h - NAMES engine: formula -> name, name -> formula, info line. */
#ifndef CHEM_NAMES_H
#define CHEM_NAMES_H

#include "work.h"

/* `in` is the typed line (letters lowercase). out = "answer\ninfo",
 * small = the other reading of an ambiguous formula ("Co: cobalt"). */
void names_compute(const char *in, char *out, char *small, work_t *w);

/* OPTIONS / LIST popups: rows to show ("NO3-  nitrate") and the text each
 * one enters ("nitrate"). Returns the row count. */
enum { NL_IONS, NL_ACIDS, NL_COMMON, NL_MONO };
#define NL_COLS 36
uint8_t names_list(uint8_t kind, char show[][NL_COLS], char enter[][NL_COLS], uint8_t max);

#endif
