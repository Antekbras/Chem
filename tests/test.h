/* tiny host test framework */
#ifndef CHEM_TEST_H
#define CHEM_TEST_H
#include <stdio.h>
#include <string.h>

static int t_fail = 0, t_pass = 0;

#define CHECK(cond, ...) do { \
    if (cond) t_pass++; \
    else { t_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

#define CHECK_STR(got, want) do { \
    const char *g_ = (got), *w_ = (want); \
    CHECK(!strcmp(g_, w_), "got \"%s\" want \"%s\"", g_, w_); \
} while (0)

#define TEST_END() do { \
    printf("%s: %d passed, %d failed\n", __FILE__, t_pass, t_fail); \
    return t_fail ? 1 : 0; \
} while (0)

/* TI chars -> readable ASCII for comparisons */
static inline const char *ascii(const char *s) {
    static char buf[8][128];
    static int k = 0;
    char *o = buf[k = (k + 1) & 7], *p = o;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == 0x1B) *p++ = 'E';
        else if (c == 0x1A) *p++ = '-';
        else if (c == 0x1C) { *p++ = '>'; }
        else if (c == 0x12) *p++ = '2';
        else if (c == 0x0E) *p++ = '3';
        else if (c == 0x0C) *p++ = '.';
        else if (c == 0x14) *p++ = 'o';
        else if (c >= 0x80 && c <= 0x89) *p++ = (char)('0' + c - 0x80);
        else *p++ = (char)c;
    }
    *p = 0;
    return o;
}

/* readable -> TI chars: '>' = STO arrow, '~' = (-) negative */
static inline const char *ti(const char *s) {
    static char buf[8][128];
    static int k = 0;
    char *o = buf[k = (k + 1) & 7], *p = o;
    for (; *s; s++) {
        if (*s == '>') *p++ = 0x1C;
        else if (*s == '~') *p++ = 0x1A;
        else *p++ = *s;
    }
    *p = 0;
    return o;
}
#endif
