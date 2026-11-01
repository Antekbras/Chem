/* CONVERT: every TEST_MAP CONVERT row, MOLE/UNITCONV features, KCl */
#include "../apps/CONVERT/src/convert.c"
#include "test.h"
#include "plat_mock.h"

static void row(const char *in, const char *want) {
    char out[UI_OUT], small[UI_SMALL];
    work_t w;
    work_clear(&w);
    convert_compute(ti(in), out, small, &w);
    CHECK(!strcmp(ascii(out), want), "%s = \"%s\", want \"%s\"", in, ascii(out), want);
}

static void work_has(const char *in, const char *line) {
    char out[UI_OUT], small[UI_SMALL];
    work_t w;
    uint8_t i;
    bool ok = false;
    work_clear(&w);
    convert_compute(ti(in), out, small, &w);
    for (i = 0; i < w.n; i++) if (strstr(ascii(w.line[i]), line)) ok = true;
    CHECK(ok, "%s: no work line \"%s\"", in, line);
}

int main(void) {
    /* topic 2: area and volume units */
    row("2.45cm2>mm2", "245 mm2");
    row(".0350m3>cm3", "3.50E4 cm3");
    row("4.70km2>m2", "4.70E6 m2");
    row("525cm2>m2", "0.0525 m2");
    row("1.80cm3>mm3", "1.80E3 mm3");
    row("10.0m2>ft2", "108 ft2");
    row("2.50m3>l", "2.50E3 L");
    row("3.19m2>cm2", "3.19E4 cm2");
    /* UNITCONV: any unit, compound, prefixes, temperature, partial info */
    row("3.5mi/hr>m/s", "1.6 m/s");
    row("25.0c>k", "298 K");
    row("1.50m>cm", "150. cm");
    row("12nm>m", "1.2E-8 m");
    row("mi/hr>m/s", "0.44704 m/s");
    row("1.000mol>particles", "6.022E23 particles");
    /* MOLE: molar mass (KCl bug fixed), g <-> mol <-> particles */
    row("kcl", "74.55 g/mol");
    row("kcl>g/mol", "74.55 g/mol");
    row("cuso4.5h2o", "249.69 g/mol");
    row("25g kcl>mol", "0.34 mol");
    row("25g kcl", "0.34 mol");
    row("1.50mol h2o>g", "27.0 g");
    row("1.50mol h2o", "27.0 g");
    row("1.50mol", "9.03E23 particles");
    row("6.022E23>mol", "1.000 mol");
    row("2.00g h2o>molecules", "6.68E22 molecules");
    row("5.0lb nacl>mol", "39 mol");
    row("250mg kcl>mol", "0.0034 mol");
    row("25g 58.44g/mol>mol", "0.43 mol");
    row("1.00g h2o>atoms h", "6.68E22 atoms H");
    /* topic 15: molarity */
    row("250.0ml .640m>mol", "0.160 mol");
    row(".0500mol .250m>ml", "200. mL");
    row(".0450mol 300.0ml>m", "0.150 M");
    row("250.0ml .400m nacl>g", "5.84 g");
    row("3.00m 25.0ml>150.0ml", "0.500 M");
    row("75.0ml 2.20m>mol", "0.165 mol");
    row("4.00g naoh 500.0ml>m", "0.200 M");
    row("4.00g naoh 500.0ml", "0.200 M");
    row("160.0ml 1.85m>mol", "0.296 mol");
    /* Fall 25 / Spring 25 */
    row("50.0ml .0100m h2so4>g", "0.0490 g");
    row("2.23g c12h22o11 20.0ml>m", "0.326 M");
    row("pcl5>p", "14.87 % P");
    row("10.00g n2o4>g n", "3.045 g N");
    row("100.0ml .100m naoh>g", "0.400 g");
    row(".100m>50.0ml .0500m", "25.0 mL");
    row("5.23E24 co2>g", "382 g");
    row("fe(no3)2>fe", "31.05 % Fe");
    row("100.0g hno3>g o", "76.17 g O");
    row("100.0g h2so4>mol", "1.020 mol");
    /* letters that are both a unit and an element */
    row("h2so4>s", "32.70 % S");
    row("100.0g h2so4>mol s", "1.020 mol S");
    /* errors */
    row("25g>mol", "ERR:NEED FORMULA");
    row("2.0cm>g", "ERR:UNITS");
    row(".0450mol>m", "ERR:NEED VOLUME");
    /* show work */
    work_has("25g kcl>mol", "K  1*39.1");
    work_has("25g kcl>mol", "M KCl=74.55");
    work_has("25g kcl>mol", "n=m/M=25/74.55");
    work_has("5.23E24 co2>g", "n=N/NA=");
    work_has("3.00m 25.0ml>150.0ml", "M1V1=M2V2");
    work_has("2.00g h2o>molecules", "N=n*NA=");
    work_has("250.0ml .640m>mol", "250.0mL=0.2500 L");
    work_has("2.45cm2>mm2", "1cm2=100mm2");
    /* pick lists */
    {
        char t[24];
        convert_pick_text("KCl", false, '5', t);
        CHECK_STR(t, " kcl");
        convert_pick_text("mL", true, '5', t);
        CHECK_STR(t, "ml");
        convert_pick_text("M", true, '5', t);
        CHECK_STR(t, "M");
        convert_pick_text("CH3COOH", false, 0, t);
        CHECK_STR(t, "ch3cooh");
    }
    {   /* every list item parses */
        uint8_t i;
        for (i = 0; i < NSUBST; i++) {
            formula_t f;
            char t[24];
            convert_pick_text(subst[i], false, 0, t);
            CHECK(formula_parse(t, (uint8_t)strlen(t), FP_COMPOUND, &f), "subst %s", t);
            CHECK(!strcmp(f.disp, subst[i]), "subst %s -> %s", subst[i], f.disp);
        }
        for (i = 0; i < NUNITLIST; i++) {
            unit_t u;
            char t[24];
            convert_pick_text(unitlist[i], true, 0, t);
            CHECK(unit_parse(t, &u) == strlen(t), "unit %s", t);
        }
    }
    {   /* the app: [Y=] 5 types kcl, [WINDOW] picks a unit, ENTER computes */
        static const app_def_t d = {"CONVERT", "CONVRTst", SMART_ALL, convert_compute, convert_field, 0, 0};
        static const uint8_t pick5[] = {K_5}, pick_ml[] = {K_DOWN, K_DOWN, K_UP, K_ENTER}, cancel[] = {K_CLEAR};
        mock_state_reset();
        app_open(&d);
        app_key(&d, K_2); app_key(&d, K_5);
        mock_keys(pick_ml, 4);
        app_key(&d, K_WINDOW);
        CHECK_STR(app_entry()->buf, "25kg");
        mock_keys(pick5, 1);
        app_key(&d, K_YEQU);
        CHECK_STR(app_entry()->buf, "25kg kcl");
        mock_keys(cancel, 1);
        app_key(&d, K_YEQU);
        CHECK_STR(app_entry()->buf, "25kg kcl");
        app_key(&d, K_ENTER);
        CHECK_STR(ascii(app_screen()->out), "3.4E2 mol");
        mock_keys(pick5, 1);            /* after ENTER a pick starts a new line */
        app_key(&d, K_YEQU);
        CHECK_STR(app_entry()->buf, "kcl");
        app_key(&d, K_ENTER);
        CHECK_STR(ascii(app_screen()->out), "74.55 g/mol");
        app_close(&d);
    }
    TEST_END();
}
