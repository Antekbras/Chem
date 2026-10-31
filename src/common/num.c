/* num.c - sig-fig numbers */
#include <string.h>
#include "num.h"

num_t num_exact(fp_t v) {
    num_t n;
    n.v = v; n.sf = SF_EXACT; n.lsd = LSD_EXACT;
    return n;
}

static int8_t lsd_for(fp_t v, int sf) {
    if (v == 0) return (int8_t)(1 - sf);
    return (int8_t)(fp_exp10(v) - sf + 1);
}

/* round v to a multiple of 10^lsd, half away from zero */
static fp_t round_at(fp_t v, int lsd) {
    fp_t x, s;
    if (lsd < 0) {
        s = fp_pow10(-lsd);
        x = v * s;
        x += (x > 0 ? 1 : -1) * FP_FABS(x) * 1e-12;
        return FP_ROUND(x) / s;
    }
    s = fp_pow10(lsd);
    x = v / s;
    x += (x > 0 ? 1 : -1) * FP_FABS(x) * 1e-12;
    return FP_ROUND(x) * s;
}

num_t num_sig(fp_t v, int sf) {
    num_t n;
    if (sf >= SF_EXACT) return num_exact(v);
    if (sf < 1) sf = 1;
    n.v = v; n.sf = (int8_t)sf; n.lsd = lsd_for(v, sf);
    return n;
}

bool num_is_exact(num_t a) { return a.sf >= SF_EXACT; }

num_t num_round(num_t a) {
    if (num_is_exact(a) || a.v != a.v) return a;
    a.v = round_at(a.v, a.lsd);
    return a;
}

num_t num_with_sf(fp_t v, int sf) {
    num_t n = num_sig(v, sf);
    if (!num_is_exact(n)) {
        /* rounding can carry into a new decade: 9.996 -> 10.0 */
        fp_t r = round_at(v, n.lsd);
        n.lsd = lsd_for(r, sf);
    }
    return n;
}

static bool isnum(char c) { return c >= '0' && c <= '9'; }

uint8_t num_parse(const char *s, num_t *out) {
    const char *p = s;
    bool neg = false, point = false, any = false;
    fp_t mant = 0;
    int frac = 0, first = -1, ndig = 0, last_nz = -1, exp = 0, sf, lsd;
    if (*p == '-' || *p == CH_NEG) { neg = true; p++; }
    for (;; p++) {
        if (isnum(*p)) {
            any = true;
            mant = mant * 10 + (*p - '0');
            if (point) frac++;
            if (*p != '0') {
                if (first < 0) first = ndig;
                last_nz = ndig;
            }
            ndig++;
        } else if (*p == '.' && !point) {
            point = true;
        } else break;
    }
    if (!any) return 0;
    /* exponent: E or ᴇ followed by optional sign and digits */
    if (*p == CH_EE || *p == 'E' || *p == 'e') {
        const char *q = p + 1;
        bool eneg = false;
        int ev = 0;
        if (*q == '-' || *q == CH_NEG) { eneg = true; q++; }
        else if (*q == '+') q++;
        if (isnum(*q)) {
            while (isnum(*q)) ev = ev * 10 + (*q++ - '0');
            exp = eneg ? -ev : ev;
            p = q;
        }
    }
    mant /= fp_pow10(frac);
    if (exp) mant *= fp_pow10(exp);
    if (first < 0) {                 /* the number is zero */
        sf = 1;
        lsd = -frac + exp;
    } else if (point) {
        sf = ndig - first;
        lsd = -frac + exp;
    } else {
        sf = last_nz - first + 1;    /* trailing zeros without '.' don't count */
        lsd = (ndig - 1 - last_nz) + exp;
    }
    out->v = neg ? -mant : mant;
    out->sf = (int8_t)sf;
    out->lsd = (int8_t)lsd;
    return (uint8_t)(p - s);
}

int num_count_sf(const char *s) {
    num_t n;
    uint8_t len = num_parse(s, &n);
    if (!len || s[len]) return 0;
    return n.sf;
}

num_t num_neg(num_t a) { a.v = -a.v; return a; }

num_t num_add(num_t a, num_t b, bool round_steps) {
    num_t r;
    r.v = a.v + b.v;
    if (num_is_exact(a) && num_is_exact(b)) return num_exact(r.v);
    r.lsd = a.lsd > b.lsd ? a.lsd : b.lsd;
    if (round_steps) r.v = round_at(r.v, r.lsd);
    {
        fp_t rr = round_at(r.v, r.lsd);
        /* sf <= 0 means nothing significant is left: keep 1 */
        r.sf = (int8_t)(rr == 0 ? 1 : fp_exp10(rr) - r.lsd + 1);
        if (r.sf < 1) r.sf = 1;
    }
    return r;
}

num_t num_sub(num_t a, num_t b, bool round_steps) {
    return num_add(a, num_neg(b), round_steps);
}

static num_t finish_mul(fp_t v, int sf, bool round_steps) {
    num_t r = num_with_sf(v, sf);
    if (round_steps) r = num_round(r);
    return r;
}

num_t num_mul(num_t a, num_t b, bool round_steps) {
    return finish_mul(a.v * b.v, a.sf < b.sf ? a.sf : b.sf, round_steps);
}

num_t num_div(num_t a, num_t b, bool round_steps) {
    if (b.v == 0) { num_t r = a; r.v = NAN; return r; }
    return finish_mul(a.v / b.v, a.sf < b.sf ? a.sf : b.sf, round_steps);
}

num_t num_pow(num_t a, num_t b, bool round_steps) {
    return finish_mul(FP_POW(a.v, b.v), a.sf, round_steps);
}

/* ---- formatting ---- */

/* decimal digits of round(|v| / 10^lsd); returns count, most significant first */
static int digits(fp_t v, int lsd, char *d) {
    unsigned long long n;
    int k = 0, i;
    char t[24];
    fp_t x = FP_FABS(round_at(v, lsd));
    x = lsd < 0 ? x * fp_pow10(-lsd) : x / fp_pow10(lsd);
    n = (unsigned long long)(x + 0.5);
    do { t[k++] = (char)('0' + n % 10); n /= 10; } while (n && k < 22);
    for (i = 0; i < k; i++) d[i] = t[k - 1 - i];
    return k;
}

static char *put_int(char *p, int v) {
    char t[6];
    int k = 0;
    if (v < 0) { *p++ = CH_NEG; v = -v; }
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (k) *p++ = t[--k];
    return p;
}

/* precision used for display: exact numbers show up to 10 digits */
static void disp_prec(num_t a, int *sf, int *lsd) {
    if (num_is_exact(a)) {
        *sf = 10;
        *lsd = lsd_for(a.v, 10);
    } else {
        *sf = a.sf;
        *lsd = a.lsd;
    }
}

static bool bad(num_t a, char *buf, uint8_t *len) {
    if (a.v != a.v || a.v - a.v != 0) {
        strcpy(buf, a.v != a.v ? "ERR:DOMAIN" : "ERR:OVERFLOW");
        *len = (uint8_t)strlen(buf);
        return true;
    }
    return false;
}

uint8_t num_fmt_sci(num_t a, char *buf) {
    char d[24], *p = buf;
    int sf, lsd, n, e, i;
    uint8_t len;
    if (bad(a, buf, &len)) return len;
    disp_prec(a, &sf, &lsd);
    if (a.v == 0) {
        /* zero: show as many zeros as its precision */
        *p++ = '0';
        *p = 0;
        return 1;
    }
    n = digits(a.v, lsd, d);
    e = lsd + n - 1;                 /* exponent of the leading digit */
    if (num_is_exact(a)) while (n > 1 && d[n - 1] == '0') n--;
    else if (n > sf && sf > 0) n = sf;      /* carried into a new decade */
    if (a.v < 0) *p++ = CH_NEG;
    *p++ = d[0];
    if (n > 1) {
        *p++ = '.';
        for (i = 1; i < n; i++) *p++ = d[i];
    }
    if (e != 0 || !num_is_exact(a)) {
        *p++ = CH_EE;
        p = put_int(p, e);
    }
    *p = 0;
    return (uint8_t)(p - buf);
}

uint8_t num_fmt_plain(num_t a, char *buf) {
    char d[24], *p = buf;
    int sf, lsd, n, e, i;
    uint8_t len;
    if (bad(a, buf, &len)) return len;
    disp_prec(a, &sf, &lsd);
    if (a.v == 0) {
        *p++ = '0';
        if (lsd < 0) { *p++ = '.'; for (i = 0; i < -lsd; i++) *p++ = '0'; }
        *p = 0;
        return (uint8_t)(p - buf);
    }
    n = digits(a.v, lsd, d);
    e = lsd + n - 1;
    if (e > 11 || lsd < -10) return num_fmt_sci(a, buf);
    if (num_is_exact(a)) {           /* drop trailing zeros after the point */
        while (lsd < 0 && n > 1 && d[n - 1] == '0') { n--; lsd++; }
    }
    if (a.v < 0) *p++ = CH_NEG;
    if (lsd >= 0) {
        for (i = 0; i < n; i++) *p++ = d[i];
        for (i = 0; i < lsd; i++) *p++ = '0';
        /* "200." : a significant trailing zero needs the decimal point */
        if (!num_is_exact(a) && lsd == 0 && d[n - 1] == '0') *p++ = '.';
    } else {
        int intdig = n + lsd;        /* digits before the point */
        if (intdig <= 0) {
            *p++ = '0';
            *p++ = '.';
            for (i = 0; i < -intdig; i++) *p++ = '0';
            for (i = 0; i < n; i++) *p++ = d[i];
        } else {
            for (i = 0; i < intdig; i++) *p++ = d[i];
            *p++ = '.';
            for (; i < n; i++) *p++ = d[i];
        }
    }
    *p = 0;
    return (uint8_t)(p - buf);
}
