/* REACT - balance, stoichiometry, limiting reagent, yield
 * Stub on the shared library: shows the typed line auto-cased. */
#include <string.h>
#include "app.h"
#include "formula.h"

static void compute(const char *in, char *out, char *small, work_t *w) {
    char echo[64];
    (void)small;
    app_echo(in, FP_COMPOUND, echo, sizeof echo);
    strncpy(out, echo, UI_OUT - 1);
    out[UI_OUT - 1] = 0;
    work_add(w, echo, NULL, NULL, NULL);
}

int main(void) {
    static const app_def_t d = {"REACT", "REACTst", SMART_ALL, compute, 0};
    app_run(&d);
    return 0;
}
