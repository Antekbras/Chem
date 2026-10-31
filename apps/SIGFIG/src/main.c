/* SIGFIG - home-screen calculator with sig figs (stub on the shared library).
 * A lone number shows its sig-fig count. */
#include <string.h>
#include "app.h"
#include "expr.h"

static qty_t ans;
static bool has_ans;

static void compute(const char *in, char *out, char *small, work_t *w) {
    expr_ctx_t c;
    qty_t q;
    char u[24];
    int sf = num_count_sf(in);
    memset(&c, 0, sizeof c);
    c.round_steps = true;
    c.ans = ans;
    c.has_ans = has_ans;
    c.work = w;
    if (!expr_eval(in, &c, &q)) {
        strcpy(out, expr_err_str(c.err));
        return;
    }
    ans = q;
    has_ans = true;
    if (sf) {                       /* one number: count its sig figs */
        char n[24];
        num_fmt_sci(q.n, n);
        strcpy(small, n);
        out[0] = (char)('0' + sf / 10);
        out[1] = (char)('0' + sf % 10);
        strcpy(out + 2, " sig figs");
        if (out[0] == '0') memmove(out, out + 1, strlen(out));
        return;
    }
    num_fmt_sci(q.n, out);
    if (unit_str(&q.u, u)) { strcat(out, " "); strcat(out, u); }
    num_fmt_plain(q.n, small);
}

int main(void) {
    static const app_def_t d = {"SIGFIG", "SIGFIGst", SMART_CALC, compute, 0};
    app_run(&d);
    return 0;
}
