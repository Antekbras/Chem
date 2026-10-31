/* entry.c - one-line editor and key-count planner */
#include <string.h>
#include "entry.h"
#include "fp.h"

/* number-lock characters */
static char num_char(uint8_t k) {
    switch (k) {
    case K_0: return '0'; case K_1: return '1'; case K_2: return '2';
    case K_3: return '3'; case K_4: return '4'; case K_5: return '5';
    case K_6: return '6'; case K_7: return '7'; case K_8: return '8';
    case K_9: return '9';
    case K_DECPNT: return '.';
    case K_ADD: return '+';
    case K_SUB: return '-';
    case K_MUL: return '*';
    case K_DIV: return '/';
    case K_POWER: return '^';
    case K_SQUARE: return CH_SQ;
    case K_LPAREN: return '(';
    case K_RPAREN: return ')';
    case K_COMMA: return ' ';
    case K_CHS: return CH_NEG;
    case K_STO: return CH_STO;
    }
    return 0;
}

/* letters printed above the keys (letter-lock) */
static char let_char(uint8_t k) {
    switch (k) {
    case K_MATH: return 'a'; case K_APPS: return 'b'; case K_PRGM: return 'c';
    case K_RECIP: return 'd'; case K_SIN: return 'e'; case K_COS: return 'f';
    case K_TAN: return 'g'; case K_POWER: return 'h'; case K_SQUARE: return 'i';
    case K_LPAREN: return 'k'; case K_RPAREN: return 'l'; case K_DIV: return 'm';
    case K_LOG: return 'n'; case K_7: return 'o'; case K_8: return 'p';
    case K_MUL: return 'r'; case K_LN: return 's'; case K_4: return 't';
    case K_5: return 'u'; case K_6: return 'v'; case K_SUB: return 'w';
    case K_1: return 'y'; case K_2: return 'z';
    /* keys that work the same in either lock */
    case K_3: return '3'; case K_9: return '9'; case K_ADD: return '+';
    case K_DECPNT: return '.'; case K_COMMA: return ' '; case K_0: return ' ';
    case K_STO: return CH_STO; case K_CHS: return '-';
    }
    return 0;
}

/* smart lock: function keys type their letters in number-lock */
static char smart_char(uint8_t k) {
    switch (k) {
    case K_MATH: return 'a'; case K_APPS: return 'b'; case K_PRGM: return 'c';
    case K_RECIP: return 'd'; case K_SIN: return 'e'; case K_COS: return 'f';
    case K_TAN: return 'g'; case K_POWER: return 'h'; case K_SQUARE: return 'i';
    case K_LOG: return 'n'; case K_LN: return 's';
    }
    return 0;
}

char entry_char(uint8_t k, uint8_t lock, bool alpha, bool second, uint32_t smart) {
    if (second) return k == K_COMMA ? CH_EE : 0;
    if (alpha) {
        /* one real OS letter, including those the locks don't give */
        switch (k) {
        case K_COMMA: return 'j';
        case K_9: return 'q';
        case K_STO: return 'x';
        }
        return let_char(k);
    }
    if (lock == LOCK_LET) return let_char(k);
    {
        char c = smart_char(k);
        if (c && (smart & SMART_BIT(c))) return c;
    }
    return num_char(k);
}

void entry_clear(entry_t *e) {
    e->buf[0] = 0;
    e->len = e->cur = 0;
    e->done = false;
}

void entry_init(entry_t *e, uint32_t smart, uint8_t lock_default) {
    memset(e, 0, sizeof *e);
    e->smart = smart;
    e->lock = e->lock_default = lock_default;
    e->ans_ops = true;
}

void entry_set(entry_t *e, const char *text) {
    size_t n = strlen(text);
    if (n > ENTRY_MAX) n = ENTRY_MAX;
    memcpy(e->buf, text, n);
    e->buf[n] = 0;
    e->len = e->cur = (uint8_t)n;
    e->done = false;
}

void entry_done(entry_t *e) {
    strcpy(e->last, e->buf);
    e->done = true;
    e->lock = e->lock_default;
    e->second = e->alpha = false;
}

static bool is_op(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == CH_SQ || c == CH_STO;
}

static void put(entry_t *e, char c) {
    if (e->cur < e->len && !e->ins) {
        e->buf[e->cur++] = c;          /* overwrite, like the home screen */
        return;
    }
    if (e->len >= ENTRY_MAX) return;
    memmove(e->buf + e->cur + 1, e->buf + e->cur, (size_t)(e->len - e->cur + 1));
    e->buf[e->cur++] = c;
    e->len++;
}

static uint8_t insert(entry_t *e, char c) {
    uint8_t r = EK_CHANGED;
    if (e->done) {
        entry_clear(e);
        r = EK_NEWLINE;
    }
    if (e->len == 0 && e->ans_ops && is_op(c)) {
        put(e, 'A'); put(e, 'n'); put(e, 's');
    }
    put(e, c);
    return r;
}

uint8_t entry_key(entry_t *e, uint8_t k) {
    char c;
    e->key = k;
    if (k == K_2ND) { e->second = !e->second; e->alpha = false; return EK_CHANGED; }
    if (k == K_ALPHA) { e->alpha = !e->alpha; e->second = false; return EK_CHANGED; }
    if (e->second) {
        e->second = false;
        switch (k) {
        case K_MODE: return EK_QUIT;
        case K_UP: return EK_SCROLL_UP;
        case K_DOWN: return EK_SCROLL_DOWN;
        case K_LEFT: e->done = false; e->cur = 0; return EK_CHANGED;
        case K_RIGHT: e->done = false; e->cur = e->len; return EK_CHANGED;
        case K_DEL: e->ins = !e->ins; return EK_CHANGED;
        case K_ENTER: {
            uint8_t r = e->done ? EK_NEWLINE : EK_CHANGED;
            char tmp[ENTRY_MAX + 1];
            strcpy(tmp, e->last);
            entry_set(e, tmp);
            return r;
        }
        case K_CHS: {
            uint8_t r = EK_CHANGED;
            if (e->done) { entry_clear(e); r = EK_NEWLINE; }
            put(e, 'A'); put(e, 'n'); put(e, 's');
            return r;
        }
        }
        c = entry_char(k, e->lock, false, true, e->smart);
        if (!c) return EK_CHANGED;     /* 2nd indicator went away */
        return insert(e, c);
    }
    if (e->alpha) {
        e->alpha = false;
        c = entry_char(k, e->lock, true, false, e->smart);
        if (!c) return EK_CHANGED;
        return insert(e, c);
    }
    switch (k) {
    case K_UP: case K_DOWN:
        e->lock ^= 1;
        return EK_CHANGED;
    case K_LEFT:
        e->done = false;
        if (e->cur) e->cur--;
        return EK_CHANGED;
    case K_RIGHT:
        e->done = false;
        if (e->cur < e->len) e->cur++;
        return EK_CHANGED;
    case K_DEL:
        e->done = false;
        if (e->cur == e->len && e->cur) e->cur--;
        if (e->cur < e->len) {
            memmove(e->buf + e->cur, e->buf + e->cur + 1, (size_t)(e->len - e->cur));
            e->len--;
        }
        return EK_CHANGED;
    case K_CLEAR:
        if (!e->len) return EK_CLEAR_EMPTY;
        entry_clear(e);
        e->lock = e->lock_default;
        return EK_CHANGED;
    case K_ENTER: return EK_ENTER;
    case K_GRAPH: return EK_GRAPH;
    case K_YEQU: e->fkey = 0; return EK_FKEY;
    case K_WINDOW: e->fkey = 1; return EK_FKEY;
    case K_ZOOM: e->fkey = 2; return EK_FKEY;
    case K_TRACE: e->fkey = 3; return EK_FKEY;
    }
    c = entry_char(k, e->lock, false, false, e->smart);
    if (!c) return EK_OTHER;
    return insert(e, c);
}

/* ---- planner ---- */

typedef struct { uint8_t pre, key, lock_after, cost; } opt_t;

/* ways to type c from `lock`; returns count */
static uint8_t options(char c, uint8_t lock, uint32_t smart, opt_t *o) {
    uint8_t n = 0, k;
    for (k = 1; k < K_MAX && n < 8; k++) {
        uint8_t extra = c == CH_STO ? 1 : 0;   /* [STO→] is not data */
        if (entry_char(k, lock, false, false, smart) == c) {
            o[n].pre = 0; o[n].key = k; o[n].lock_after = lock; o[n].cost = extra; n++;
        } else if (entry_char(k, (uint8_t)(lock ^ 1), false, false, smart) == c) {
            o[n].pre = K_UP; o[n].key = k; o[n].lock_after = (uint8_t)(lock ^ 1); o[n].cost = (uint8_t)(1 + extra); n++;
        } else if (entry_char(k, lock, true, false, smart) == c) {
            o[n].pre = K_ALPHA; o[n].key = k; o[n].lock_after = lock; o[n].cost = 1; n++;
        } else if (entry_char(k, lock, false, true, smart) == c) {
            o[n].pre = K_2ND; o[n].key = k; o[n].lock_after = lock; o[n].cost = 1; n++;
        }
    }
    return n;
}

static uint8_t plan_line(const char *s, uint8_t len, uint32_t smart, uint8_t *keys, uint8_t max, uint8_t *extra) {
    /* cost[i][lock]: min extra keys to type s[0..i) ending in lock */
    uint8_t cost[ENTRY_MAX + 1][2], from[ENTRY_MAX + 1][2], via[ENTRY_MAX + 1][2][2];
    uint8_t i, l, n = 0, lock, best;
    opt_t o[8];
    if (len > ENTRY_MAX) return 0;
    memset(cost, 0xFF, sizeof cost);
    cost[0][LOCK_NUM] = 0;
    for (i = 0; i < len; i++) {
        for (l = 0; l < 2; l++) {
            uint8_t no, j;
            if (cost[i][l] == 0xFF) continue;
            no = options(s[i], l, smart, o);
            for (j = 0; j < no; j++) {
                uint8_t c = (uint8_t)(cost[i][l] + o[j].cost), la = o[j].lock_after;
                if (c < cost[i + 1][la]) {
                    cost[i + 1][la] = c;
                    from[i + 1][la] = l;
                    via[i + 1][la][0] = o[j].pre;
                    via[i + 1][la][1] = o[j].key;
                }
            }
        }
    }
    lock = cost[len][0] <= cost[len][1] ? 0 : 1;
    best = cost[len][lock];
    if (best == 0xFF) return 0;
    /* walk back */
    {
        uint8_t seq[2 * ENTRY_MAX + 2], m = 0;
        for (i = len; i > 0; i--) {
            seq[m++] = via[i][lock][1];
            if (via[i][lock][0]) seq[m++] = via[i][lock][0];
            lock = from[i][lock];
        }
        while (m) {
            if (keys && n < max) keys[n] = seq[m - 1];
            n++; m--;
        }
    }
    if (keys && n < max) keys[n] = K_ENTER;
    n++;
    *extra = (uint8_t)(*extra + best + 1);
    return n;
}

uint8_t entry_plan(const char *text, uint32_t smart, uint8_t *keys, uint8_t max, uint8_t *extra) {
    uint8_t total = 0;
    const char *p = text;
    *extra = 0;
    for (;;) {
        const char *sep = strstr(p, " ; ");
        uint8_t len = (uint8_t)(sep ? (size_t)(sep - p) : strlen(p)), n;
        n = plan_line(p, len, smart, keys ? keys + total : NULL,
                      (uint8_t)(max > total ? max - total : 0), extra);
        if (!n) return 0;
        total = (uint8_t)(total + n);
        if (!sep) break;
        p = sep + 3;
    }
    return total;
}
