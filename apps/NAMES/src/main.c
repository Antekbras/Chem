/* NAMES - formula -> name and name -> formula (replaces NOMENC).
 * Type either one; ENTER shows the answer and an info line under it.
 * OPTIONS: lists of ions, acids, common names; LIST: polyatomic ions.
 * Picking a row looks it up. WORK shows how it was worked out. */
#include "app.h"
#include "names.h"

#define NL_MAX 64
static char show[NL_MAX][NL_COLS], enter[NL_MAX][NL_COLS];
static const char *rows[NL_MAX];

static void pick_list(uint8_t kind, const char *title) {
    uint8_t i, n = names_list(kind, show, enter, NL_MAX);
    for (i = 0; i < n; i++) rows[i] = show[i];
    i = ui_pick(title, rows, n);
    if (i != 0xFF) app_enter(enter[i]);
}

static void field(uint8_t n) {
    static const char *const opt[4] = {
        "Polyatomic ions", "Acids", "Common names", "Monatomic anions"};
    static const char *const title[4] = {"POLYATOMIC IONS", "ACIDS", "COMMON NAMES", "ANIONS"};
    static uint8_t last;
    if (n < 2) {                                  /* OPTIONS */
        n = ui_pick_at("OPTIONS", opt, 4, last);
        if (n == 0xFF) return;
        last = n;
    } else n = NL_IONS;                           /* LIST */
    pick_list(n, title[n]);
}

int main(void) {
    static const char *const labels[5] = {"OPTIONS", "OPTIONS", "LIST", "WORK", "QUIT"};
    static const app_def_t d = {"NAMES", "NAMESst", SMART_ALL, names_compute, field, 0, 0,
                                "CHEMICAL NAMES", "nacl  iron3 oxide  nitrous acid"};
    app_labels(labels);
    app_run(&d);
    return 0;
}
