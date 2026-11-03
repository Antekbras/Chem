/* ions, solubility rules, electrolyte class */
#include "../src/common/ionic.h"
#include "test.h"

static void ion(const char *t, const char *cat, int qc, int nc, const char *an, int qa, int na, int sol, int cls) {
    formula_t f;
    ionic_t o;
    bool ok;
    formula_parse(t, (uint8_t)strlen(t), FP_COMPOUND, &f);
    ok = ionic_split(&f, &o);
    if (cat) {
        CHECK(ok && !strcmp(o.cat, cat) && o.qc == qc && o.nc == nc && !strcmp(o.an, an) &&
              o.qa == qa && o.na == na, "%s: %s%+d x%d %s%+d x%d", t, o.cat, o.qc, o.nc, o.an, o.qa, o.na);
        CHECK(ionic_soluble(&o) == (sol != 0), "%s soluble", t);
    } else CHECK(!ok, "%s molecular", t);
    CHECK(ionic_class(&f, &o) == cls, "%s class %d", t, ionic_class(&f, &o));
}

int main(void) {
    char b[16];
    ion("nacl", "Na", 1, 1, "Cl", -1, 1, 1, EL_SALT);
    ion("fe(no3)2", "Fe", 2, 1, "NO3", -1, 2, 1, EL_SALT);
    ion("fe2o3", "Fe", 3, 2, "O", -2, 3, 0, EL_INSOLUBLE);
    ion("cuno3", "Cu", 1, 1, "NO3", -1, 1, 1, EL_SALT);
    ion("(nh4)2so4", "NH4", 1, 2, "SO4", -2, 1, 1, EL_SALT);
    ion("nh4f", "NH4", 1, 1, "F", -1, 1, 1, EL_SALT);
    ion("baso4", "Ba", 2, 1, "SO4", -2, 1, 0, EL_INSOLUBLE);
    ion("agcl", "Ag", 1, 1, "Cl", -1, 1, 0, EL_INSOLUBLE);
    ion("pbi2", "Pb", 2, 1, "I", -1, 2, 0, EL_INSOLUBLE);
    ion("cu2s", "Cu", 1, 2, "S", -2, 1, 0, EL_INSOLUBLE);
    ion("caco3", "Ca", 2, 1, "CO3", -2, 1, 0, EL_INSOLUBLE);
    ion("k2so4", "K", 1, 2, "SO4", -2, 1, 1, EL_SALT);
    ion("koh", "K", 1, 1, "OH", -1, 1, 1, EL_STRONG_BASE);
    ion("mg(oh)2", "Mg", 2, 1, "OH", -1, 2, 0, EL_INSOLUBLE);
    ion("na2o2", "Na", 1, 2, "O2", -2, 1, 1, EL_SALT);
    ion("mno2", "Mn", 4, 1, "O", -2, 2, 0, EL_INSOLUBLE);
    ion("hcl", "H", 1, 1, "Cl", -1, 1, 1, EL_STRONG_ACID);
    ion("h2so4", "H", 1, 2, "SO4", -2, 1, 1, EL_STRONG_ACID);
    ion("h3po4", "H", 1, 3, "PO4", -3, 1, 1, EL_WEAK_ACID);
    ion("hf", "H", 1, 1, "F", -1, 1, 1, EL_WEAK_ACID);
    ion("hc2h3o2", "H", 1, 1, "C2H3O2", -1, 1, 1, EL_WEAK_ACID);
    ion("nh3", 0, 0, 0, 0, 0, 0, 1, EL_WEAK_BASE);
    ion("h2o", 0, 0, 0, 0, 0, 0, 1, EL_NONELEC);
    ion("c12h22o11", 0, 0, 0, 0, 0, 0, 1, EL_NONELEC);
    ion("ccl4", 0, 0, 0, 0, 0, 0, 1, EL_NONELEC);
    ion_str("SO4", -2, b); CHECK_STR(b, "SO4 2-");
    ion_str("Fe", 3, b); CHECK_STR(b, "Fe3+");
    ion_str("Cl", -1, b); CHECK_STR(b, "Cl-");
    TEST_END();
}
