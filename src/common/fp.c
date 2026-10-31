#include "fp.h"

fp_t fp_pow10(int n) {
    fp_t r = 1, b = 10;
    bool neg = n < 0;
    if (neg) n = -n;
    while (n) {               /* exact for small n */
        if (n & 1) r *= b;
        b *= b;
        n >>= 1;
    }
    return neg ? 1 / r : r;
}

int fp_exp10(fp_t v) {
    int e;
    v = FP_FABS(v);
    if (v == 0) return 0;
    e = (int)FP_FLOOR(FP_LOG10(v));
    if (fp_pow10(e) > v * (1 + 1e-12)) e--;
    else if (fp_pow10(e + 1) <= v * (1 + 1e-12)) e++;
    return e;
}
