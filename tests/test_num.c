/* sig-fig numbers and the SIGFIG expression evaluator: every topic-1 row */
#include "../src/common/expr.h"
#include "test.h"

static void row(const char *text, const char *want) {
    expr_ctx_t c;
    qty_t q;
    char buf[40];
    memset(&c, 0, sizeof c);
    c.round_steps = true;
    if (!expr_eval(ti(text), &c, &q)) { CHECK(0, "%s: error %d", text, c.err); return; }
    num_fmt_plain(q.n, buf);
    CHECK(!strcmp(ascii(buf), want), "%s = %s, want %s", text, ascii(buf), want);
}

static void count(const char *text, int want) {
    int got = num_count_sf(text);
    CHECK(got == want, "sf(%s) = %d, want %d", text, got, want);
}

static void fmt(fp_t v, int sf, const char *sci, const char *plain) {
    char a[40], b[40];
    num_t n = num_with_sf(v, sf);
    num_fmt_sci(n, a);
    num_fmt_plain(n, b);
    CHECK_STR(ascii(a), sci);
    CHECK_STR(ascii(b), plain);
}

/* main answer + alternate form: plain when it shows the sig figs */
static void ans(const char *in, const char *m, const char *a) {
    num_t n;
    char mm[32], aa[32];
    num_parse(ti(in), &n);
    num_fmt_answer(n, mm, aa);
    CHECK(!strcmp(ascii(mm), m) && !strcmp(ascii(aa), a), "%s: got %s | %s want %s | %s",
          in, ascii(mm), ascii(aa), m, a);
}

int main(void) {
    /* TEST_MAP topic 1, Day 1-7 + Prep */
    row("(4.82*3.0100)/(15.2-2.11)", "1.11");      /* D1 */
    row("(8.314*2.0)/(0.500+12.31)", "1.3");       /* D2 */
    row("(25.0+1.236)*0.0402", "1.05");            /* D3 */
    row("(102.5-99.8)/(3.141*2.00)", "0.43");      /* D4 */
    row("6.02*10.00+1.3", "61.5");                 /* D5 */
    row(".004050*12.0/3.7", "0.013");              /* D6 */
    row("(14.2+0.31-1.0)/2.500", "5.40");          /* D7 */
    row("6.0509*5.000/(9.8+4.70)", "2.09");        /* P  */
    /* home-screen niceties */
    row("2.0(3.00)", "6.0");
    row("~2.0*3", "-6");
    row("(1.20", "1.20");
    row("2.0E3*1.5", "3000");
    row("2.0E3*1.50", "3000");
    row("1.0-0.98", "0.0");

    /* counting sig figs of one number */
    count("0.004050", 4);
    count("1500", 2);
    count("1500.", 4);
    count("200.", 3);
    count("2.50E3", 3);
    count(".640", 3);
    count("100.0", 4);
    count("6.022E23", 4);
    count("0", 1);
    count("12a", 0);

    /* answer formats: sci with sig figs + plain decimal */
    fmt(245, 3, "2.45E2", "245");
    fmt(0.0131, 2, "1.3E-2", "0.013");
    fmt(200, 3, "2.00E2", "200.");
    fmt(5.404, 3, "5.40E0", "5.40");
    fmt(9.996, 3, "1.00E1", "10.0");
    fmt(4.7e6, 3, "4.70E6", "4700000");
    fmt(-0.0525, 3, "-5.25E-2", "-0.0525");
    fmt(500, 1, "5E2", "500");
    {
        char b[40];
        num_fmt_plain(num_exact(74.55), b);
        CHECK_STR(b, "74.55");
        num_fmt_sci(num_exact(6.022e23), b);
        CHECK_STR(ascii(b), "6.022E23");
    }

    /* Ans: an operator on an empty line continues from the last answer */
    {
        expr_ctx_t c;
        qty_t q;
        char b[40];
        memset(&c, 0, sizeof c);
        c.round_steps = true;
        expr_eval("13.9", &c, &q);
        c.ans = q; c.has_ans = true;
        CHECK(expr_eval("Ans*.835", &c, &q), "Ans*");
        num_fmt_plain(q.n, b);
        CHECK_STR(b, "11.6");
    }
    /* units ride along: 2.50E3*1.2/3.456kg -> 8.7E2 /kg */
    {
        expr_ctx_t c;
        qty_t q;
        char b[40], u[40];
        work_t w;
        memset(&c, 0, sizeof c);
        c.round_steps = true;
        c.work = &w;
        CHECK(expr_eval("2.50E3*1.2/3.456kg", &c, &q), "units expr");
        num_fmt_sci(q.n, b);
        unit_str(&q.u, u);
        CHECK_STR(ascii(b), "8.7E2");
        CHECK_STR(u, "1/kg");
        CHECK(w.n == 2, "work lines %d", w.n);
        CHECK(expr_eval("3.5mi/hr+1.0m/s", &c, &q), "unit add");
        unit_str(&q.u, u);
        CHECK_STR(u, "mi/hr");
        CHECK(!expr_eval("2g+3s", &c, &q) && c.err == EXPR_UNITS, "unit mismatch");
        CHECK(!expr_eval("2/0", &c, &q) && c.err == EXPR_DIV0, "div0");
        CHECK(!expr_eval("2**3", &c, &q) && c.err == EXPR_SYNTAX, "syntax");
    }
    ans("5.890", "5.890", "");
    ans("61.5", "61.5", "6.15E1");
    ans("0.013", "0.013", "1.3E-2");
    ans("200.", "200.", "2.00E2");
    ans("0.00045", "4.5E-4", "0.00045");
    ans("1.50E3", "1.50E3", "1500");
    ans("6.022E23", "6.022E23", "");
    TEST_END();
}
