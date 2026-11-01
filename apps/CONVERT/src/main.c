/* CONVERT - any unit to any unit, g/mol/particles, molar mass, molarity,
 * dilution, % and element mass in a compound. One line, [STO→] = "to".
 * [Y=] common substances  [WINDOW] units  [GRAPH] show work */
#include "app.h"
#include "convert.h"

int main(void) {
    static const app_def_t d = {"CONVERT", "CONVRTst", SMART_ALL, convert_compute, convert_field, 0, 0};
    app_run(&d);
    return 0;
}
