/* formula parser: auto-case, molar masses, ambiguity, ions, hydrates */
#include "../src/common/formula.h"
#include "../src/common/num.h"
#include "test.h"

static void mm(const char *text, const char *disp, const char *mass) {
    formula_t f;
    char b[40], m[24];
    CHECK(formula_parse(text, (uint8_t)strlen(text), FP_COMPOUND, &f), "%s parse", text);
    formula_str(&f, b);
    CHECK_STR(ascii(b), disp);
    num_fmt_plain(num_sig(formula_mass(&f), (int)strlen(mass) - 1), m);
    CHECK(!strcmp(m, mass), "%s mass %s want %s", text, m, mass);
}

static void name(const char *text, uint8_t prefer, const char *disp, int alt) {
    formula_t f;
    char b[40];
    CHECK(formula_parse(text, (uint8_t)strlen(text), prefer, &f), "%s parse", text);
    formula_str(&f, b);
    CHECK_STR(ascii(b), disp);
    CHECK(f.alt_z == alt, "%s alt %d want %d", text, f.alt_z, alt);
}

int main(void) {
    /* brief: required molar masses (KCl bug fixed: never K+C+I) */
    mm("kcl", "KCl", "74.55");
    mm("c12h22o11", "C12H22O11", "342.30");
    mm("h2so4", "H2SO4", "98.08");
    mm("fe(no3)2", "Fe(NO3)2", "179.86");
    mm("n2o4", "N2O4", "92.02");
    mm("cuso4.5h2o", "CuSO4.5H2O", "249.69");
    mm("CuSO4*5H2O", "CuSO4.5H2O", "249.69");
    mm("KCl", "KCl", "74.55");
    /* other masses from the practice tests */
    mm("nacl", "NaCl", "58.44");
    mm("naoh", "NaOH", "40.00");
    mm("kclo3", "KClO3", "122.55");
    mm("cuo", "CuO", "79.55");
    mm("hno3", "HNO3", "63.02");
    mm("pcl5", "PCl5", "208.22");

    /* auto-case on every formula in TEST_MAP */
    {
        static const char *const in[] = {
            "tio2", "pbo2", "febr3", "sncl4", "cu2o", "v2o5", "mn2o7", "mns3",
            "n2o4", "p4o10", "sf6", "cl2o7", "s4cl2", "ccl4", "cabr2", "(nh4)2so4",
            "pcl3", "baso4", "c12h22o11", "c3h8", "o2", "co2", "h2o", "p4", "al",
            "al2o3", "c4h10", "fe", "cl2", "fecl3", "nh3", "no", "s8", "so2", "so3",
            "n2o5", "no2", "alcl3", "b2h6", "b2o3", "h3po4", "hclo4", "fe2o3",
            "pbr3", "f2", "p4f5", "br2", "zn", "hcl", "zncl2", "h2", "cu", "c8h18",
            "h2so4", "sn", "al(no3)3", "hno3", "koh", "na2s", "cuno3", "kclo4",
            "nh4f", "k2so4", "kcl", "naoh", "k2o", "kno3", "b", "bcl3", "n2",
            "ch4", "cuo", "kclo3", "c7h6o", "c2h4o", "c9h8o", "h3p", "cus", "clo3",
            "h2co3", "na2co3", "pb(no3)2", "agno3", "nacl", "hf", "ni", "pcl5", 0};
        static const char *const out[] = {
            "TiO2", "PbO2", "FeBr3", "SnCl4", "Cu2O", "V2O5", "Mn2O7", "MnS3",
            "N2O4", "P4O10", "SF6", "Cl2O7", "S4Cl2", "CCl4", "CaBr2", "(NH4)2SO4",
            "PCl3", "BaSO4", "C12H22O11", "C3H8", "O2", "CO2", "H2O", "P4", "Al",
            "Al2O3", "C4H10", "Fe", "Cl2", "FeCl3", "NH3", "NO", "S8", "SO2", "SO3",
            "N2O5", "NO2", "AlCl3", "B2H6", "B2O3", "H3PO4", "HClO4", "Fe2O3",
            "PBr3", "F2", "P4F5", "Br2", "Zn", "HCl", "ZnCl2", "H2", "Cu", "C8H18",
            "H2SO4", "Sn", "Al(NO3)3", "HNO3", "KOH", "Na2S", "CuNO3", "KClO4",
            "NH4F", "K2SO4", "KCl", "NaOH", "K2O", "KNO3", "B", "BCl3", "N2",
            "CH4", "CuO", "KClO3", "C7H6O", "C2H4O", "C9H8O", "H3P", "CuS", "ClO3",
            "H2CO3", "Na2CO3", "Pb(NO3)2", "AgNO3", "NaCl", "HF", "Ni", "PCl5"};
        int i;
        for (i = 0; in[i]; i++) {
            formula_t f;
            char b[40];
            bool ok = formula_parse(in[i], (uint8_t)strlen(in[i]), FP_COMPOUND, &f);
            formula_str(&f, b);
            CHECK(ok && !strcmp(b, out[i]), "%s -> %s want %s", in[i], b, out[i]);
        }
    }

    /* ambiguous: formula apps take the compound, ATOM the element */
    name("co", FP_COMPOUND, "CO", 27);
    name("cs", FP_COMPOUND, "CS", 55);
    name("hf", FP_COMPOUND, "HF", 72);
    name("no", FP_COMPOUND, "NO", 102);
    name("co", FP_ELEMENT, "Co", 0);
    name("hf", FP_ELEMENT, "Hf", 0);
    name("cr", FP_ELEMENT, "Cr", 0);

    /* ions (ATOM topic 3) and polyatomic ions */
    name("fe3+", FP_ELEMENT, "Fe3+", 0);
    name("cu2+", FP_ELEMENT, "Cu2+", 0);
    name("co2+", FP_ELEMENT, "Co2+", 0);
    name("co3+", FP_COMPOUND, "Co3+", 0);
    name("o2-", FP_COMPOUND, "O2-", 0);
    name("cl-", FP_COMPOUND, "Cl-", 0);
    name("no3-", FP_COMPOUND, "NO3-", 0);
    name("so42-", FP_COMPOUND, "SO4 2-", 0);
    name("so4 2-", FP_COMPOUND, "SO4 2-", 0);
    name("nh4+", FP_COMPOUND, "NH4+", 0);
    name("po43-", FP_COMPOUND, "PO4 3-", 0);
    name("hg22+", FP_COMPOUND, "Hg2 2+", 0);
    {
        formula_t f;
        formula_parse("fe3+", 4, FP_ELEMENT, &f);
        CHECK(f.charge == 3 && f.nel == 1 && f.el[0].z == 26, "fe3+ fields");
        formula_parse("so42-", 5, FP_COMPOUND, &f);
        CHECK(f.charge == -2 && formula_count(&f, 8) == 4, "so42- fields");
    }

    /* structure for NAMES and show-work */
    {
        formula_t f;
        char b[40];
        formula_parse("(nh4)2so4", 9, FP_COMPOUND, &f);
        CHECK(f.npart == 3 && f.part[0].group && f.part[0].mult == 2, "(NH4)2SO4 parts");
        CHECK(formula_count(&f, 1) == 8 && formula_count(&f, 7) == 2, "(NH4)2SO4 counts");
        formula_parse("cuso4.5h2o", 10, FP_COMPOUND, &f);
        CHECK(f.hydrate == 5 && f.npart == 3 && formula_count(&f, 8) == 9, "hydrate");
        formula_parse("2h2o", 4, FP_COMPOUND, &f);
        CHECK(f.coef == 2 && formula_count(&f, 1) == 2, "coefficient");
        formula_parse("fe(no3)2", 8, FP_COMPOUND, &f);
        formula_work_line(&f, 0, b);
        CHECK_STR(b, "Fe 1*55.85=55.85");
        formula_work_line(&f, 2, b);
        CHECK_STR(b, "O  6*15.999=95.994");
        formula_pretty("Fe(NO3)2", b);
        CHECK_STR(ascii(b), "Fe(NO3)2");
        CHECK((unsigned char)b[5] == 0x83 && (unsigned char)b[7] == 0x82, "subscripts");
    }
    /* rejects */
    {
        formula_t f;
        CHECK(!formula_parse("xyz", 3, FP_COMPOUND, &f), "xyz");
        CHECK(!formula_parse("fe(no3", 6, FP_COMPOUND, &f), "unbalanced");
        CHECK(!formula_parse("", 0, FP_COMPOUND, &f), "empty");
        CHECK(!formula_parse("h2-o", 4, FP_COMPOUND, &f), "bad char");
    }
    TEST_END();
}
