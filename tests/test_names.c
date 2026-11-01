/* NAMES: every TEST_MAP question, both directions, and NOMENC's tables */
#include "../apps/NAMES/src/names.c"
#include "test.h"

static void nm(const char *in, const char *want, const char *info) {
    char out[UI_OUT], small[UI_SMALL], *nl;
    work_t w;
    work_clear(&w);
    names_compute(in, out, small, &w);
    nl = strrchr(out, '\n');
    if (nl) *nl++ = 0;
    CHECK(!strcmp(ascii(out), want), "%s: got \"%s\" want \"%s\"", in, ascii(out), want);
    if (info) CHECK(nl && !strcmp(ascii(nl), info), "%s info: got \"%s\" want \"%s\"", in, nl ? ascii(nl) : "", info);
    CHECK(!nl || strlen(nl) <= UI_COLS, "%s info too long", in);
    CHECK(w.n > 0 || !strcmp(out, "not recognized"), "%s: no work", in);
}

int main(void) {
    /* TEST_MAP: formula -> name */
    nm("tio2", "titanium(IV) oxide", "ionic, insoluble");
    nm("pbo2", "lead(IV) oxide", NULL);
    nm("febr3", "iron(III) bromide", "ionic, soluble: Fe3+ Br-");
    nm("sncl4", "tin(IV) chloride", NULL);
    nm("cu2o", "copper(I) oxide", NULL);
    nm("v2o5", "vanadium(V) oxide", NULL);
    nm("mn2o7", "manganese(VII) oxide", NULL);
    nm("mns3", "manganese(VI) sulfide", NULL);
    nm("n2o4", "dinitrogen tetroxide", NULL);
    nm("p4o10", "tetraphosphorus decoxide", NULL);
    nm("sf6", "sulfur hexafluoride", "covalent, octahedral");
    nm("co", "carbon monoxide", "covalent, linear");
    nm("cl2o7", "dichlorine heptoxide", NULL);
    nm("s4cl2", "tetrasulfur dichloride", NULL);
    nm("ccl4", "carbon tetrachloride", "covalent, tetrahedral");
    nm("cabr2", "calcium bromide", "ionic, soluble: Ca2+ Br-");
    nm("(nh4)2so4", "ammonium sulfate", "ionic, sol.: NH4+ SO4 2-");
    nm("pcl3", "phosphorus trichloride", "covalent, trig. pyramidal");
    nm("baso4", "barium sulfate", "ionic, insoluble");
    nm("c12h22o11", "sucrose", "covalent, nonelectrolyte");
    nm("k2so4", "potassium sulfate", "ionic, soluble: K+ SO4 2-");
    nm("h3p", "hydrophosphoric acid", "weak acid");
    nm("cus", "copper(II) sulfide", "ionic, insoluble");
    nm("clo3", "chlorine trioxide", NULL);
    nm("hf", "hydrofluoric acid", "weak acid");
    nm("nh3", "ammonia", "weak base, trig. pyramidal");
    /* TEST_MAP: name -> formula */
    nm("diphosphorus pentasulfide", "P2S5", "covalent, nonelectrolyte");
    nm("disulfur dichloride", "S2Cl2", NULL);
    nm("iron3 oxide", "Fe2O3", "ionic, insoluble");
    nm("nitrous acid", "HNO2", "weak acid");

    /* NOMENC: ionic, Roman numerals, polyatomic ions, hydrates */
    nm("iron(iii) oxide", "Fe2O3", NULL);
    nm("iron (iii) oxide", "Fe2O3", NULL);
    nm("iron iii oxide", "Fe2O3", NULL);
    nm("copper2 sulfate pentahydrate", "CuSO4.5H2O", NULL);
    nm("cuso4.5h2o", "copper(II) sulfate\npentahydrate", NULL);
    nm("ammonium sulfate", "(NH4)2SO4", NULL);
    nm("calcium phosphate", "Ca3(PO4)2", "ionic, insoluble");
    nm("ca3(po4)2", "calcium phosphate", NULL);
    nm("fe(no3)2", "iron(II) nitrate", NULL);
    nm("sodium bicarbonate", "NaHCO3", NULL);
    nm("nahco3", "sodium hydrogen carbonate", NULL);
    nm("mercury(i) chloride", "Hg2Cl2", "ionic, insoluble");
    nm("hg2cl2", "mercury(I) chloride", NULL);
    nm("na2o2", "sodium peroxide", NULL);
    nm("mno2", "manganese(IV) oxide", NULL);
    nm("kmno4", "potassium permanganate", NULL);
    nm("nh4cl", "ammonium chloride", NULL);
    nm("naoh", "sodium hydroxide", "ionic, strong base");
    nm("agcl", "silver chloride", "ionic, insoluble");
    nm("nac2h3o2", "sodium acetate", NULL);
    nm("k2cr2o7", "potassium dichromate", NULL);
    nm("nan3", "sodium azide", NULL);
    nm("na3c6h5o7", "sodium citrate", NULL);
    /* acids */
    nm("hcl", "hydrochloric acid", "strong acid: H+ Cl-");
    nm("h2so4", "sulfuric acid", "strong acid: H+ SO4 2-");
    nm("h2so3", "sulfurous acid", "weak acid");
    nm("h3po4", "phosphoric acid", NULL);
    nm("hclo", "hypochlorous acid", NULL);
    nm("hclo4", "perchloric acid", NULL);
    nm("hcn", "hydrocyanic acid", NULL);
    nm("hn3", "hydrazoic acid", NULL);
    nm("h2s", "hydrosulfuric acid", NULL);
    nm("hc2h3o2", "acetic acid", NULL);
    nm("sulfuric acid", "H2SO4", NULL);
    nm("hydrochloric acid", "HCl", NULL);
    nm("phosphorous acid", "H3PO3", NULL);
    nm("hydrosulfuric acid", "H2S", NULL);
    nm("acetic acid", "CH3COOH", "weak acid");
    nm("hydrogen chloride", "HCl", NULL);
    nm("hydrogen sulfide", "H2S", NULL);
    /* covalent, common names, elements, ions */
    nm("carbon dioxide", "CO2", "covalent, linear");
    nm("carbon monoxide", "CO", NULL);
    nm("dinitrogen tetroxide", "N2O4", NULL);
    nm("xenon tetrafluoride", "XeF4", "covalent, sq. planar");
    nm("dihydrogen monoxide", "H2O", NULL);
    nm("h2o", "water", "covalent, bent");
    nm("water", "H2O", NULL);
    nm("no", "nitrogen monoxide", NULL);
    nm("nitric oxide", "NO", NULL);
    nm("n2o", "dinitrogen monoxide", NULL);
    nm("ch4", "methane", NULL);
    nm("fe", "iron", "metal, ion 3+ 2+");
    nm("na", "sodium", "metal, ion 1+");
    nm("nh4h2po4", "ammonium dihydrogen\nphosphate", "ionic, sol.: NH4+ H2PO4-");
    nm("ca(oh)2", "calcium hydroxide", "ionic, strong base");
    nm("iron", "Fe", NULL);
    nm("oxygen", "O2", NULL);
    nm("o2", "oxygen", NULL);
    nm("fe3+", "iron(III) ion", "monatomic cation");
    nm("iron3", "Fe3+", NULL);
    nm("so42-", "sulfate ion", "polyatomic, tetrahedral");
    nm("nitrate", "NO3-", "polyatomic, trig. planar");
    nm("chloride", "Cl-", NULL);
    nm("ammonium", "NH4+", NULL);
    nm("bisulfate", "HSO4-", NULL);
    nm("hg22+", "mercury(I) ion", NULL);
    nm("tin", "Sn", NULL);
    nm("xyz", "not recognized", NULL);
    nm("h+", "hydrogen ion", NULL);
    nm("hydrogen ion", "H+", NULL);
    nm("iron(ii) ion", "Fe2+", NULL);
    nm("oxide ion", "O2-", NULL);
    nm("copper chloride", "CuCl2", NULL);

    /* the other reading of an ambiguous formula goes in the small text */
    {
        char out[UI_OUT], small[UI_SMALL];
        work_t w;
        names_compute("hf", out, small, &w);
        CHECK_STR(small, "Hf: hafnium");
    }
    /* every ion in the table has a name both ways */
    {
        uint8_t i;
        for (i = 0; i < NION; i++) {
            char out[UI_OUT], small[UI_SMALL], in[32];
            work_t w;
            strcpy(in, ion(i)->name);
            names_compute(in, out, small, &w);
            CHECK(strncmp(out, "not", 3), "ion name %s", in);
        }
    }
    TEST_END();
}
