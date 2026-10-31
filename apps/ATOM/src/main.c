/* ATOM - configs of atoms/ions/Z, element card, isotopes, light.
 * Stub on the shared library: element or ion -> electron configuration. */
#include <string.h>
#include "app.h"
#include "formula.h"

static void compute(const char *in, char *out, char *small, work_t *w) {
    formula_t f;
    econfig_t c;
    uint8_t z = 0, len = (uint8_t)strlen(in);
    if (in[0] >= '0' && in[0] <= '9' && len <= 3) {        /* atomic number */
        uint8_t i;
        for (i = 0; i < len; i++) z = (uint8_t)(z * 10 + in[i] - '0');
        f.charge = 0;
    } else if (formula_parse(in, len, FP_ELEMENT, &f) && f.nel == 1 && f.el[0].n == 1) {
        z = f.el[0].z;
    }
    if (z < 1 || z > NUM_ELEMENTS || !econfig(z, f.charge, &c)) {
        app_echo(in, FP_ELEMENT, out, UI_OUT);
        return;
    }
    econfig_str(&c, false, out);
    strcpy(small, elements[z].sym);
    work_add(w, elements[z].name, NULL, NULL, NULL);
    {
        char full[80];
        econfig_str(&c, true, full);
        work_add(w, full, NULL, NULL, NULL);
        if (strlen(full) > WORK_COLS) work_add(w, full + WORK_COLS, NULL, NULL, NULL);
    }
}

int main(void) {
    static const app_def_t d = {"ATOM", "ATOMst", SMART_ALL, compute, 0};
    app_run(&d);
    return 0;
}
