/* element table and electron configurations (TEST_MAP topics 3 and 4) */
#include "../src/common/data.h"
#include "../src/common/formula.h"
#include <stdlib.h>
#include "test.h"

static void cfg(const char *text, const char *want, int dcount) {
    formula_t f;
    econfig_t c;
    char b[64];
    uint8_t z;
    if (text[0] >= '0' && text[0] <= '9') { z = (uint8_t)atoi(text); f.charge = 0; }
    else {
        CHECK(formula_parse(text, (uint8_t)strlen(text), FP_ELEMENT, &f) && f.nel == 1, "%s", text);
        z = f.el[0].z;
    }
    CHECK(econfig(z, f.charge, &c), "%s config", text);
    econfig_str(&c, false, b);
    CHECK(!strcmp(b, want), "%s: %s want %s", text, b, want);
    if (dcount >= 0) CHECK(econfig_dcount(&c) == dcount, "%s d=%d want %d", text, econfig_dcount(&c), dcount);
}

int main(void) {
    int z;
    /* topic 3: d electrons in an ion */
    cfg("fe3+", "[Ar]3d5", 5);
    cfg("cu2+", "[Ar]3d9", 9);
    cfg("ni2+", "[Ar]3d8", 8);
    cfg("cr3+", "[Ar]3d3", 3);
    cfg("zn2+", "[Ar]3d10", 10);
    cfg("co3+", "[Ar]3d6", 6);
    cfg("ti2+", "[Ar]3d2", 2);
    cfg("co2+", "[Ar]3d7", 7);
    /* topic 4: configurations incl. exceptions */
    cfg("cr", "[Ar]4s1 3d5", 5);
    cfg("cu", "[Ar]4s1 3d10", 10);
    cfg("ag", "[Kr]5s1 4d10", 10);
    cfg("fe", "[Ar]4s2 3d6", 6);
    cfg("ni", "[Ar]4s2 3d8", 8);
    cfg("mn", "[Ar]4s2 3d5", 5);
    cfg("29", "[Ar]4s1 3d10", 10);
    cfg("mo", "[Kr]5s1 4d5", 5);
    cfg("pd", "[Kr]4d10", 10);
    cfg("au", "[Xe]6s1 4f14 5d10", 10);
    cfg("h", "1s1", 0);
    cfg("ne", "[He]2s2 2p6", 0);
    cfg("o2-", "[He]2s2 2p6", 0);
    cfg("k+", "[Ne]3s2 3p6", 0);
    cfg("cl-", "[Ne]3s2 3p6", 0);
    cfg("pb", "[Xe]6s2 4f14 5d10 6p2", 10);
    {
        econfig_t c;
        char b[80];
        econfig(26, 0, &c);
        econfig_str(&c, true, b);
        CHECK_STR(b, "1s2 2s2 2p6 3s2 3p6 4s2 3d6");
        CHECK(!econfig(1, 2, &c), "H2+ impossible");
    }

    /* table sanity */
    CHECK(elem_find("Cl", 2) == 17 && elem_find("cl", 2) == 17 && elem_find("CL", 2) == 17, "Cl");
    CHECK(elem_find("k", 1) == 19 && elem_find("j", 1) == 0, "K / J");
    CHECK(elem_find_name("iron", 4) == 26 && elem_find_name("sulphur", 7) == 16, "names");
    for (z = 1; z <= NUM_ELEMENTS; z++) {
        CHECK(elem_find(elements[z].sym, (uint8_t)strlen(elements[z].sym)) == z, "symbol %d", z);
        CHECK(elements[z].mass1000 > 1000 && (z == 1 || (long)elements[z].mass1000 > (long)elements[z - 1].mass1000 - 3000), "mass %d", z);
        CHECK(elem_find_name(elements[z].name, (uint8_t)strlen(elements[z].name)) == z, "name %d", z);
    }
    CHECK(elements[17].en100 == 316 && elements[9].en100 == 398, "EN");
    CHECK(elem_is_metal(26) && !elem_is_metal(8) && elements[14].cat == CAT_METALLOID, "categories");
    CHECK(elements[26].ions[0] == 3 && elements[26].ions[1] == 2, "Fe ions");
    CHECK(polyion_find("NO3", 3) >= 0 && polyions[polyion_find("SO4", 3)].charge == -2, "polyions");
    CHECK(polyion_find_name("ammonium", 8) == 0, "ammonium");
    TEST_END();
}
