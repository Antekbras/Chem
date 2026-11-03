/* CONVERT - any unit to any unit, g/mol/particles, molar mass, molarity,
 * dilution, % and element mass in a compound. One line, [STO→] = "to".
 * Tabs: [Y=] SUBST common substances  [WINDOW] UNITS  [TRACE] WORK  [GRAPH] QUIT */
#include "app.h"
#include "convert.h"

int main(void) {
    static const char *const labels[5] = {"SUBST", "UNITS", "", "WORK", "QUIT"};
    static const app_def_t d = {"CONVERT", "CONVRTst", SMART_ALL, convert_compute, convert_field, 0, 0, "UNIT CONVERTER", "25g kcl\x1Cmol  2.45cm2\x1Cmm2  kcl"};
    app_labels(labels);
    app_run(&d);
    return 0;
}
