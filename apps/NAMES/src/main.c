/* NAMES - formula -> name and name -> formula (replaces NOMENC).
 * Type either one; ENTER shows the answer and an info line under it,
 * [GRAPH] shows how it was worked out. */
#include "app.h"
#include "names.h"

int main(void) {
    static const app_def_t d = {"NAMES", "NAMESst", SMART_ALL, names_compute, 0, 0, 0};
    app_run(&d);
    return 0;
}
