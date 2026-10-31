/* expr.c - recursive-descent evaluator over qty_t */
#include <string.h>
#include "expr.h"

typedef struct {
    const char *s, *p;
    expr_ctx_t *c;
} ep_t;

static bool expr(ep_t *e, qty_t *out);

static void skip(ep_t *e) { while (*e->p == ' ') e->p++; }

static bool fail(ep_t *e, uint8_t err) {
    if (!e->c->err) { e->c->err = err; e->c->errpos = (uint8_t)(e->p - e->s); }
    return false;
}

static void log_op(ep_t *e, const qty_t *a, char op, const qty_t *b, const qty_t *r) {
    char x[24], y[24], z[24];
    char o[2];
    if (!e->c->work) return;
    num_fmt_plain(a->n, x);
    num_fmt_plain(b->n, y);
    num_fmt_plain(r->n, z);
    o[0] = op; o[1] = 0;
    {
        char line[WORK_COLS + 1];
        strncpy(line, x, WORK_COLS); line[WORK_COLS] = 0;
        strncat(line, o, WORK_COLS - strlen(line));
        strncat(line, y, WORK_COLS - strlen(line));
        work_add(e->c->work, line, "=", z, NULL);
    }
}

static bool is_unit_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void opt_unit(ep_t *e, qty_t *q) {
    unit_t u;
    uint8_t n;
    if (!is_unit_start(*e->p)) return;
    n = unit_parse(e->p, &u);
    if (!n) return;
    unit_mul(&q->u, &u, 1);
    e->p += n;
}

static bool primary(ep_t *e, qty_t *q) {
    skip(e);
    memset(&q->u, 0, sizeof q->u);
    if (*e->p == '(') {
        e->p++;
        if (!expr(e, q)) return false;
        skip(e);
        if (*e->p == ')') e->p++;      /* closing paren optional at end, like TI */
        else if (*e->p) return fail(e, EXPR_SYNTAX);
        opt_unit(e, q);
        return true;
    }
    if (!strncmp(e->p, "Ans", 3) || !strncmp(e->p, "ans", 3)) {
        if (!e->c->has_ans) return fail(e, EXPR_SYNTAX);
        *q = e->c->ans;
        e->p += 3;
        return true;
    }
    {
        uint8_t n = num_parse(e->p, &q->n);
        if (!n || *e->p == '-' || *e->p == CH_NEG) return fail(e, EXPR_SYNTAX);
        e->p += n;
        opt_unit(e, q);
        return true;
    }
}

static bool combine(ep_t *e, qty_t *a, char op, qty_t *b) {
    bool r = e->c->round_steps;
    qty_t res;
    res.u = a->u;
    switch (op) {
    case '+': case '-': {
        if (!unit_is_empty(&b->u) || !unit_is_empty(&a->u)) {
            fp_t v;
            if (unit_is_empty(&a->u) || unit_is_empty(&b->u)) return fail(e, EXPR_UNITS);
            unit_resolve_pair(&b->u, &a->u);
            if (!unit_convert(b->n.v, &b->u, &a->u, &v)) return fail(e, EXPR_UNITS);
            if (v != b->n.v) b->n = num_with_sf(v, b->n.sf);
        }
        res.n = op == '+' ? num_add(a->n, b->n, r) : num_sub(a->n, b->n, r);
        break;
    }
    case '*':
        res.n = num_mul(a->n, b->n, r);
        unit_mul(&res.u, &b->u, 1);
        break;
    case '/':
        if (b->n.v == 0) return fail(e, EXPR_DIV0);
        res.n = num_div(a->n, b->n, r);
        unit_mul(&res.u, &b->u, -1);
        break;
    case '^': {
        int k, pw = (int)b->n.v;
        res.n = num_pow(a->n, b->n, r);
        if (!unit_is_empty(&a->u)) {
            if ((fp_t)pw != b->n.v) return fail(e, EXPR_UNITS);
            for (k = 1; k < pw; k++) unit_mul(&res.u, &a->u, 1);
        }
        break;
    }
    }
    log_op(e, a, op, b, &res);
    *a = res;
    return true;
}

static bool unary(ep_t *e, qty_t *q);

static bool power(ep_t *e, qty_t *q) {
    if (!primary(e, q)) return false;
    for (;;) {
        skip(e);
        if (*e->p == CH_SQ) {
            qty_t two;
            e->p++;
            two.n = num_exact(2); memset(&two.u, 0, sizeof two.u);
            if (!combine(e, q, '^', &two)) return false;
        } else if (*e->p == '^') {
            qty_t b;
            e->p++;
            if (!unary(e, &b)) return false;   /* right side may be negative */
            if (!combine(e, q, '^', &b)) return false;
        } else break;
    }
    return true;
}

static bool unary(ep_t *e, qty_t *q) {
    skip(e);
    if (*e->p == CH_NEG || *e->p == '-') {
        e->p++;
        if (!unary(e, q)) return false;
        q->n = num_neg(q->n);
        return true;
    }
    return power(e, q);
}

static bool term(ep_t *e, qty_t *q) {
    if (!unary(e, q)) return false;
    for (;;) {
        char op;
        qty_t b;
        skip(e);
        op = *e->p;
        if (op == '*' || op == '/') e->p++;
        else if (op == '(' || (op >= '0' && op <= '9') || op == '.') op = '*';  /* implicit */
        else break;
        if (!unary(e, &b)) return false;
        if (!combine(e, q, op, &b)) return false;
    }
    return true;
}

static bool expr(ep_t *e, qty_t *q) {
    if (!term(e, q)) return false;
    for (;;) {
        char op;
        qty_t b;
        skip(e);
        op = *e->p;
        if (op != '+' && op != '-') break;
        e->p++;
        if (!term(e, &b)) return false;
        if (!combine(e, q, op, &b)) return false;
    }
    return true;
}

bool expr_starts_with_op(const char *s) {
    while (*s == ' ') s++;
    return *s == '+' || *s == '-' || *s == '*' || *s == '/' || *s == '^' || *s == CH_SQ;
}

bool expr_eval(const char *s, expr_ctx_t *c, qty_t *out) {
    ep_t e;
    qty_t q;
    e.s = e.p = s;
    e.c = c;
    c->err = 0;
    c->errpos = 0;
    work_clear(c->work);
    skip(&e);
    if (!*e.p) return fail(&e, EXPR_EMPTY);
    if (expr_starts_with_op(e.p)) {
        char op = *e.p;
        if (!c->has_ans) return fail(&e, EXPR_SYNTAX);
        q = c->ans;
        if (op == CH_SQ) {
            qty_t two;
            e.p++;
            two.n = num_exact(2); memset(&two.u, 0, sizeof two.u);
            if (!combine(&e, &q, '^', &two)) return false;
        } else {
            qty_t b;
            e.p++;
            if (op == '+' || op == '-') { if (!term(&e, &b)) return false; }
            else if (op == '^') { if (!unary(&e, &b)) return false; }
            else { if (!unary(&e, &b)) return false; }
            if (!combine(&e, &q, op, &b)) return false;
        }
        /* continue the rest of the line left to right */
        for (;;) {
            char o2;
            qty_t b;
            skip(&e);
            o2 = *e.p;
            if (o2 == '+' || o2 == '-') { e.p++; if (!term(&e, &b)) return false; }
            else if (o2 == '*' || o2 == '/' || o2 == '^') { e.p++; if (!unary(&e, &b)) return false; }
            else break;
            if (!combine(&e, &q, o2, &b)) return false;
        }
    } else if (!expr(&e, &q)) {
        return false;
    }
    skip(&e);
    if (*e.p) return fail(&e, EXPR_SYNTAX);
    q.n = num_round(q.n);
    *out = q;
    return true;
}

const char *expr_err_str(uint8_t err) {
    switch (err) {
    case EXPR_SYNTAX: return "ERR:SYNTAX";
    case EXPR_UNITS:  return "ERR:UNITS";
    case EXPR_DIV0:   return "ERR:DIVIDE BY 0";
    case EXPR_EMPTY:  return "";
    }
    return "";
}
