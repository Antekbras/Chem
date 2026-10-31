/* app.c - echo (pure part of the app shell) */
#include <string.h>
#include "app.h"
#include "units.h"
#include "formula.h"

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
