/* REACT: every TEST_MAP REACT row gives the answer key's result.
 * Rows of the answer are joined by '|', '>' is the → arrow. */
#define main react_main
#include "../apps/REACT/src/main.c"
#undef main
#include "test.h"

static work_t w;

static const char *run(const char *line) {
    static char b[UI_OUT];
    char out[UI_OUT], small[UI_SMALL], *p;
    out[0] = small[0] = 0;
    work_clear(&w);
    compute(ti(line), out, small, &w);
    strcpy(b, ascii(out));
    for (p = b; *p; p++) if (*p == '\n') *p = '|';
    return b;
}

static void eq(const char *line, const char *want) {
    const char *got = run(line);
    CHECK(!strcmp(got, want), "%s: got \"%s\" want \"%s\"", line, got, want);
}

/* show-work has this line */
static void work_has(const char *want) {
    int k;
    for (k = 0; k < w.n; k++) if (!strcmp(ascii(w.line[k]), want)) return;
    CHECK(0, "work line \"%s\" missing", want);
}

int main(void) {
    /* topic 21: atom counts on [GRAPH] */
    eq("c3h8+o2>co2+h2o", "C3H8+5O2>3CO2+4H2O|sum 13 combustion");
    work_has("O  10 = 10");
    eq("p4+o2>p4o10", "P4+5O2>P4O10|sum 7 combination");
    work_has("O  10 = 10");
    eq("al+o2>al2o3", "4Al+3O2>2Al2O3|sum 9 combination");
    work_has("O  6 = 6");
    eq("c4h10+o2>co2+h2o", "2C4H10+13O2>8CO2+10H2O|sum 33 combustion");
    work_has("O  26 = 26");
    eq("fe+cl2>fecl3", "2Fe+3Cl2>2FeCl3|sum 7 combination");
    work_has("Cl 6 = 6");
    eq("nh3+o2>no+h2o", "4NH3+5O2>4NO+6H2O|sum 19");
    work_has("H  12 = 12");
    eq("s8+o2>so2", "S8+8O2>8SO2|sum 17 combination");
    work_has("O  16 = 16");
    eq("s8+o2>so3", "S8+12O2>8SO3|sum 21 combination");
    work_has("O  24 = 24");
    /* topic 22: coefficients */
    eq("n2o5>no2+o2", "2N2O5>4NO2+O2|sum 7 decomposition");
    eq("al+cl2>alcl3", "2Al+3Cl2>2AlCl3|sum 7 combination");
    eq("b2h6+o2>b2o3+h2o", "B2H6+3O2>B2O3+3H2O|sum 8");
    eq("p4o10+h2o>h3po4", "P4O10+6H2O>4H3PO4|sum 11 combination");
    eq("no+o2>no2", "2NO+O2>2NO2|sum 5 combination");
    eq("cl2o7+h2o>hclo4", "Cl2O7+H2O>2HClO4|sum 4 combination");
    eq("fe2o3+co>fe+co2", "Fe2O3+3CO>2Fe+3CO2|sum 9");
    eq("pbr3+f2>p4f5+br2", "8PBr3+5F2>2P4F5+12Br2|sum 27 single displacement");
    /* topics 23-25: type, activity series, products */
    eq("zn+hcl>zncl2+h2", "Zn+2HCl>ZnCl2+H2|sum 5 single displacement|spectators Cl-");
    eq("cu+hcl", "no reaction|Cu below H2");
    eq("c3h8+o2", "C3H8+5O2>3CO2+4H2O|sum 13 combustion");
    /* Fall 25 / Spring 25 */
    eq("h2+n2>nh3", "3H2+N2>2NH3|sum 6 combination");
    eq("c8h18+o2", "2C8H18+25O2>16CO2+18H2O|sum 61 combustion");
    eq("fe+h2so4", "Fe+H2SO4>FeSO4+H2|sum 4 single displacement|spectators SO4 2-");
    eq("sn+al(no3)3", "no reaction|Sn below Al");
    eq("hno3+koh", "HNO3+KOH>H2O+KNO3|sum 4 neutralization|spectators NO3- K+");
    eq("na2s+cuno3", "Na2S+2CuNO3>2NaNO3+Cu2S|sum 6 double displacement|spectators Na+ NO3-|ppt Cu2S");
    eq("kclo4+nh4f", "no reaction|all ions stay in water");
    eq("kcl+naoh", "no reaction|all ions stay in water");
    eq("k2o+al(no3)3>al2o3+kno3", "3K2O+2Al(NO3)3>Al2O3+6KNO3|sum 12 double displacement|spectators K+ NO3-|ppt Al2O3");
    eq("n2+h2o>n2o4+n2h4", "3N2+4H2O>N2O4+2N2H4|sum 10");
    eq("4mol b+6mol cl2>bcl3", "none left|4 mol BCl3");
    eq("163kg ch4+o2>co2+h2o>mol o2", "2.03E4 mol O2");
    eq("12.0mol n2+30.0mol h2>nh3", "H2 limiting|2.0 mol N2 left|20.0 mol NH3");
    eq("150.0g cuo+nh3>cu+n2+h2o>g n2", "17.61 g N2");
    eq("35.5g kclo3>kcl+o2>g o2", "13.9 g O2");
    eq("Ans*.835", "11.6 g");
    eq("100.0g c7h6o+c2h4o>c9h8o+h2o>100.0g c9h8o", "80.30 %");
    work_has("124.5 g C9H8O theor.");
    eq("h2co3+naoh>na2co3+h2o", "H2CO3+2NaOH>Na2CO3+2H2O|sum 6 neutralization|spectators Na+");
    eq("fe+pb(no3)2", "Fe+Pb(NO3)2>Fe(NO3)2+Pb|sum 4 single displacement|spectators NO3-");
    eq("naoh+h3po4", "3NaOH+H3PO4>Na3PO4+3H2O|sum 8 neutralization|spectators Na+");
    eq("agno3+nacl", "AgNO3+NaCl>AgCl+NaNO3|sum 4 double displacement|spectators NO3- Na+|ppt AgCl");
    eq("al+hcl", "2Al+6HCl>2AlCl3+3H2|sum 13 single displacement|spectators Cl-");
    eq("ni+hcl", "Ni+2HCl>NiCl2+H2|sum 5 single displacement|spectators Cl-");

    /* more predictions */
    eq("kclo3", "2KClO3>2KCl+3O2|sum 7 decomposition");
    eq("caco3", "CaCO3>CaO+CO2|sum 3 decomposition");
    eq("na+h2o", "2Na+2H2O>2NaOH+H2|sum 7 single displacement");
    eq("cl2+nabr", "Cl2+2NaBr>2NaCl+Br2|sum 6 single displacement|spectators Na+");
    eq("i2+nacl", "no reaction|I2 below Cl2");
    eq("na+cl2", "2Na+Cl2>2NaCl|sum 5 combination");
    eq("ag++cl->agcl", "Ag++Cl->AgCl|sum 3");
    eq("xe+he", "type >products");
    /* stoichiometry variants: solution volume x molarity, products predicted */
    eq("25.0ml .100m hcl+naoh>g h2o", "0.0450 g H2O");
    eq("10.0g h2+10.0g o2>h2o", "O2 limiting|8.7 g H2 left|11.3 g H2O");
    eq("1.00E24 h2+o2>h2o>g h2o", "29.9 g H2O");
    /* plain math, errors */
    eq("2+3", "5");
    eq("h2+o2>h2o>g h2o>g", "ERR:SYNTAX");
    eq("fe+o2>cu", "ERR:CAN'T BALANCE");
    eq("fe+o2>fe2o3>g fe2o3", "ERR:NO AMOUNT");
    TEST_END();
}
