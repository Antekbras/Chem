/* SIGFIG - home-screen calculator with sig figs (stub on the shared library).
 * A lone number shows its sig-fig count. */
#include <string.h>
#include "app.h"
#include "expr.h"

/* Ans survives closing the app (saved with the app state) */
static struct { qty_t ans; bool has_ans; } mem;

static void compute(const char *in, char *out, char *small, work_t *w) {
    expr_ctx_t c;
    qty_t q;
    char u[24];
    int sf = num_count_sf(in);
    memset(&c, 0, sizeof c);
    c.round_steps = true;
    c.ans = mem.ans;
    c.has_ans = mem.has_ans;
    c.work = w;
    if (!expr_eval(in, &c, &q)) {
        strcpy(out, expr_err_str(c.err));
        return;
    }
    mem.ans = q;
    mem.has_ans = true;
    if (sf) {                       /* one number: count its sig figs */
        char *p = out;
        if (sf >= 10) *p++ = (char)('0' + sf / 10);
        *p++ = (char)('0' + sf % 10);
        strcpy(p, sf == 1 ? " sig fig" : " sig figs");
        num_fmt_sci(q.n, small);
        return;
    }
    {
        char main_[32], alt[32], line[64];
        num_fmt_answer(q.n, main_, alt);
        strcpy(line, main_);
        if (unit_str(&q.u, u)) { strcat(line, " "); strcat(line, u); }
        strncpy(out, line, UI_OUT - 1);
        out[UI_OUT - 1] = 0;
        strncpy(small, alt, UI_SMALL - 1);
        small[UI_SMALL - 1] = 0;
    }
}

int main(void) {
    static const app_def_t d = {"SIGFIG", "SIGFIGst", SMART_CALC, compute, 0, &mem, sizeof mem};
    app_run(&d);
    return 0;
}
