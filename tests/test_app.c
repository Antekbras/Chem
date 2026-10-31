/* app_echo: the stubs show the typed line auto-cased */
#include "../src/common/app.h"
#include "../src/common/formula.h"
#include "test.h"

static void echo(const char *in, uint8_t prefer, const char *want) {
    char o[96];
    app_echo(ti(in), prefer, o, sizeof o);
    CHECK_STR(ascii(o), want);
}

int main(void) {
    echo("250.0ml .640m nacl>g", FP_COMPOUND, "250.0mL .640m NaCl>g");
    echo("2.45cm2>mm2", FP_COMPOUND, "2.45cm2>mm2");
    echo("c3h8+o2>co2+h2o", FP_COMPOUND, "C3H8+O2>CO2+H2O");
    echo("4mol b+6mol cl2>bcl3", FP_COMPOUND, "4mol B+6mol Cl2>BCl3");
    echo("fe3+", FP_ELEMENT, "Fe3+");
    echo("co", FP_ELEMENT, "Co");
    echo("co", FP_COMPOUND, "CO");
    echo("kcl+naoh", FP_COMPOUND, "KCl+NaOH");
    echo("Ans*.835", FP_COMPOUND, "Ans*.835");
    {
        char o[8];
        app_echo("c12h22o11", FP_COMPOUND, o, sizeof o);
        CHECK(strlen(o) == 7, "clipped");
    }
    TEST_END();
}
