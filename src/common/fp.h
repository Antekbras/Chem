/* fp.h - floating point type shared by all modules.
 * On the calculator (CEdev) `double` is only 32-bit, so we use the 64-bit
 * `long double`. On the host, plain 64-bit `double` gives identical results. */
#ifndef CHEM_FP_H
#define CHEM_FP_H

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __TICE__
typedef long double fp_t;
#define FP_LOG10 log10l
#define FP_POW   powl
#define FP_FLOOR floorl
#define FP_FABS  fabsl
#define FP_ROUND roundl
#else
typedef double fp_t;
#define FP_LOG10 log10
#define FP_POW   pow
#define FP_FLOOR floor
#define FP_FABS  fabs
#define FP_ROUND round
#endif

/* 10^n for small integer n, exact for |n| <= 22 */
fp_t fp_pow10(int n);
/* floor(log10(|v|)), v != 0; corrected for rounding error at powers of 10 */
int fp_exp10(fp_t v);

/* TI-OS large-font character codes used in buffers and on screen. */
#define CH_NEG   '\x1A'  /* (-) negative sign  */
#define CH_EE    '\x1B'  /* E exponent (2nd+,) */
#define CH_STO   '\x1C'  /* -> store arrow     */
#define CH_SQ    '\x12'  /* superscript 2      */
#define CH_CUBE  '\x0E'  /* superscript 3      */
#define CH_SUB0  '\x80'  /* subscript 0; 1..9 follow */
#define CH_DOT   '\x0C'  /* middle dot (hydrates) */

#endif
