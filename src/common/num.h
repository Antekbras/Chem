/* num.h - numbers that carry significant figures.
 *
 * Rules (TEST_MAP topic 1):
 *   x and / : result keeps the fewest sig figs of the operands
 *   + and - : result keeps the fewest decimal places (highest last digit)
 *   mixed   : each step is rounded to its own precision before the next
 *             step when `round_steps` is on (SIGFIG default)
 *   exact counts and constants never limit (sf = SF_EXACT)
 */
#ifndef CHEM_NUM_H
#define CHEM_NUM_H

#include "fp.h"

#define SF_EXACT 99
#define LSD_EXACT (-99)

typedef struct {
    fp_t v;
    int8_t sf;    /* significant figures, SF_EXACT for exact numbers */
    int8_t lsd;   /* power of ten of the last significant digit */
} num_t;

num_t num_exact(fp_t v);
/* measured value with `sf` sig figs */
num_t num_sig(fp_t v, int sf);
/* Parse a typed number: "0.004050", "1500", "200.", ".640", "2.5E3", "6.02ᴇ23".
 * Leading '-' or CH_NEG allowed. Returns characters used, 0 if not a number. */
uint8_t num_parse(const char *s, num_t *out);
/* sig figs of a single typed number, 0 if text is not a number */
int num_count_sf(const char *s);

bool num_is_exact(num_t a);
num_t num_round(num_t a);           /* round value to its precision */
num_t num_neg(num_t a);
num_t num_add(num_t a, num_t b, bool round_steps);
num_t num_sub(num_t a, num_t b, bool round_steps);
num_t num_mul(num_t a, num_t b, bool round_steps);
num_t num_div(num_t a, num_t b, bool round_steps);  /* b.v == 0 -> v = NAN */
num_t num_pow(num_t a, num_t b, bool round_steps);  /* exponent treated as exact */
/* apply sig figs from a measured operand to a value computed elsewhere */
num_t num_with_sf(fp_t v, int sf);

/* Answer formats. Exponent uses CH_EE, negative signs use CH_NEG.
 * sci:   "2.45ᴇ2", "1.3ᴇ⁻2", "5ᴇ2"
 * plain: "245", "0.013", "200.", or sci if the number is too big/small. */
uint8_t num_fmt_sci(num_t a, char *buf);
uint8_t num_fmt_plain(num_t a, char *buf);
/* The answer as the apps show it. main: plain decimal when it shows the sig
 * figs unambiguously and isn't tiny or huge (61.5, 0.013, 200., 5.890),
 * else sci (1.5E3 for 2-sf 1500, 4.5E-4). alt: the other form when it adds
 * something (sci for 0.013 -> 1.3E-2, plain for 1.5E3 -> 1500), else "". */
void num_fmt_answer(num_t a, char *main, char *alt);

#endif
