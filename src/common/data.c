/* data.c - element table, electron configurations, polyatomic ions. */
#include <string.h>
#include "data.h"

/* generated element table: symbol, category, EN x100, mass x1000, name, ions */
const element_t elements[NUM_ELEMENTS + 1] = {
    {"", 0, 0, 0, "", {0, 0, 0}},
    {"H", CAT_NONMETAL, 220, 1008UL, "hydrogen", {1, -1, 0}}, /* 1 */
    {"He", CAT_NOBLE, 0, 4003UL, "helium", {0, 0, 0}}, /* 2 */
    {"Li", CAT_METAL, 98, 6940UL, "lithium", {1, 0, 0}}, /* 3 */
    {"Be", CAT_METAL, 157, 9012UL, "beryllium", {2, 0, 0}}, /* 4 */
    {"B", CAT_METALLOID, 204, 10810UL, "boron", {3, 0, 0}}, /* 5 */
    {"C", CAT_NONMETAL, 255, 12011UL, "carbon", {4, -4, 0}}, /* 6 */
    {"N", CAT_NONMETAL, 304, 14010UL, "nitrogen", {-3, 0, 0}}, /* 7 */
    {"O", CAT_NONMETAL, 344, 15999UL, "oxygen", {-2, 0, 0}}, /* 8 */
    {"F", CAT_NONMETAL, 398, 19000UL, "fluorine", {-1, 0, 0}}, /* 9 */
    {"Ne", CAT_NOBLE, 0, 20180UL, "neon", {0, 0, 0}}, /* 10 */
    {"Na", CAT_METAL, 93, 22990UL, "sodium", {1, 0, 0}}, /* 11 */
    {"Mg", CAT_METAL, 131, 24310UL, "magnesium", {2, 0, 0}}, /* 12 */
    {"Al", CAT_METAL, 161, 26980UL, "aluminum", {3, 0, 0}}, /* 13 */
    {"Si", CAT_METALLOID, 190, 28090UL, "silicon", {4, -4, 0}}, /* 14 */
    {"P", CAT_NONMETAL, 219, 30970UL, "phosphorus", {-3, 0, 0}}, /* 15 */
    {"S", CAT_NONMETAL, 258, 32065UL, "sulfur", {-2, 0, 0}}, /* 16 */
    {"Cl", CAT_NONMETAL, 316, 35450UL, "chlorine", {-1, 0, 0}}, /* 17 */
    {"Ar", CAT_NOBLE, 0, 39950UL, "argon", {0, 0, 0}}, /* 18 */
    {"K", CAT_METAL, 82, 39100UL, "potassium", {1, 0, 0}}, /* 19 */
    {"Ca", CAT_METAL, 100, 40080UL, "calcium", {2, 0, 0}}, /* 20 */
    {"Sc", CAT_METAL, 136, 44960UL, "scandium", {3, 0, 0}}, /* 21 */
    {"Ti", CAT_METAL, 154, 47870UL, "titanium", {4, 3, 2}}, /* 22 */
    {"V", CAT_METAL, 163, 50940UL, "vanadium", {5, 4, 3}}, /* 23 */
    {"Cr", CAT_METAL, 166, 52000UL, "chromium", {3, 2, 6}}, /* 24 */
    {"Mn", CAT_METAL, 155, 54940UL, "manganese", {2, 3, 4}}, /* 25 */
    {"Fe", CAT_METAL, 183, 55850UL, "iron", {3, 2, 0}}, /* 26 */
    {"Co", CAT_METAL, 188, 58930UL, "cobalt", {2, 3, 0}}, /* 27 */
    {"Ni", CAT_METAL, 191, 58690UL, "nickel", {2, 3, 0}}, /* 28 */
    {"Cu", CAT_METAL, 190, 63550UL, "copper", {2, 1, 0}}, /* 29 */
    {"Zn", CAT_METAL, 165, 65380UL, "zinc", {2, 0, 0}}, /* 30 */
    {"Ga", CAT_METAL, 181, 69720UL, "gallium", {3, 0, 0}}, /* 31 */
    {"Ge", CAT_METALLOID, 201, 72630UL, "germanium", {4, 0, 0}}, /* 32 */
    {"As", CAT_METALLOID, 218, 74920UL, "arsenic", {-3, 3, 5}}, /* 33 */
    {"Se", CAT_NONMETAL, 255, 78970UL, "selenium", {-2, 0, 0}}, /* 34 */
    {"Br", CAT_NONMETAL, 296, 79900UL, "bromine", {-1, 0, 0}}, /* 35 */
    {"Kr", CAT_NOBLE, 300, 83800UL, "krypton", {0, 0, 0}}, /* 36 */
    {"Rb", CAT_METAL, 82, 85470UL, "rubidium", {1, 0, 0}}, /* 37 */
    {"Sr", CAT_METAL, 95, 87620UL, "strontium", {2, 0, 0}}, /* 38 */
    {"Y", CAT_METAL, 122, 88910UL, "yttrium", {3, 0, 0}}, /* 39 */
    {"Zr", CAT_METAL, 133, 91220UL, "zirconium", {4, 0, 0}}, /* 40 */
    {"Nb", CAT_METAL, 160, 92910UL, "niobium", {5, 3, 0}}, /* 41 */
    {"Mo", CAT_METAL, 216, 95950UL, "molybdenum", {6, 4, 3}}, /* 42 */
    {"Tc", CAT_METAL, 190, 98000UL, "technetium", {7, 4, 0}}, /* 43 */
    {"Ru", CAT_METAL, 220, 101070UL, "ruthenium", {3, 4, 0}}, /* 44 */
    {"Rh", CAT_METAL, 228, 102910UL, "rhodium", {3, 0, 0}}, /* 45 */
    {"Pd", CAT_METAL, 220, 106420UL, "palladium", {2, 4, 0}}, /* 46 */
    {"Ag", CAT_METAL, 193, 107870UL, "silver", {1, 0, 0}}, /* 47 */
    {"Cd", CAT_METAL, 169, 112410UL, "cadmium", {2, 0, 0}}, /* 48 */
    {"In", CAT_METAL, 178, 114820UL, "indium", {3, 0, 0}}, /* 49 */
    {"Sn", CAT_METAL, 196, 118710UL, "tin", {4, 2, 0}}, /* 50 */
    {"Sb", CAT_METALLOID, 205, 121760UL, "antimony", {3, 5, 0}}, /* 51 */
    {"Te", CAT_METALLOID, 210, 127600UL, "tellurium", {-2, 0, 0}}, /* 52 */
    {"I", CAT_NONMETAL, 266, 126900UL, "iodine", {-1, 0, 0}}, /* 53 */
    {"Xe", CAT_NOBLE, 260, 131290UL, "xenon", {0, 0, 0}}, /* 54 */
    {"Cs", CAT_METAL, 79, 132910UL, "cesium", {1, 0, 0}}, /* 55 */
    {"Ba", CAT_METAL, 89, 137330UL, "barium", {2, 0, 0}}, /* 56 */
    {"La", CAT_METAL, 110, 138910UL, "lanthanum", {3, 0, 0}}, /* 57 */
    {"Ce", CAT_METAL, 112, 140120UL, "cerium", {3, 4, 0}}, /* 58 */
    {"Pr", CAT_METAL, 113, 140910UL, "praseodymium", {3, 0, 0}}, /* 59 */
    {"Nd", CAT_METAL, 114, 144240UL, "neodymium", {3, 0, 0}}, /* 60 */
    {"Pm", CAT_METAL, 113, 145000UL, "promethium", {3, 0, 0}}, /* 61 */
    {"Sm", CAT_METAL, 117, 150360UL, "samarium", {3, 2, 0}}, /* 62 */
    {"Eu", CAT_METAL, 120, 151960UL, "europium", {3, 2, 0}}, /* 63 */
    {"Gd", CAT_METAL, 120, 157250UL, "gadolinium", {3, 0, 0}}, /* 64 */
    {"Tb", CAT_METAL, 120, 158930UL, "terbium", {3, 0, 0}}, /* 65 */
    {"Dy", CAT_METAL, 122, 162500UL, "dysprosium", {3, 0, 0}}, /* 66 */
    {"Ho", CAT_METAL, 123, 164930UL, "holmium", {3, 0, 0}}, /* 67 */
    {"Er", CAT_METAL, 124, 167260UL, "erbium", {3, 0, 0}}, /* 68 */
    {"Tm", CAT_METAL, 125, 168930UL, "thulium", {3, 0, 0}}, /* 69 */
    {"Yb", CAT_METAL, 110, 173050UL, "ytterbium", {3, 2, 0}}, /* 70 */
    {"Lu", CAT_METAL, 127, 174970UL, "lutetium", {3, 0, 0}}, /* 71 */
    {"Hf", CAT_METAL, 130, 178490UL, "hafnium", {4, 0, 0}}, /* 72 */
    {"Ta", CAT_METAL, 150, 180950UL, "tantalum", {5, 0, 0}}, /* 73 */
    {"W", CAT_METAL, 236, 183840UL, "tungsten", {6, 0, 0}}, /* 74 */
    {"Re", CAT_METAL, 190, 186210UL, "rhenium", {7, 4, 0}}, /* 75 */
    {"Os", CAT_METAL, 220, 190230UL, "osmium", {4, 8, 0}}, /* 76 */
    {"Ir", CAT_METAL, 220, 192220UL, "iridium", {4, 3, 0}}, /* 77 */
    {"Pt", CAT_METAL, 228, 195080UL, "platinum", {2, 4, 0}}, /* 78 */
    {"Au", CAT_METAL, 254, 196970UL, "gold", {3, 1, 0}}, /* 79 */
    {"Hg", CAT_METAL, 200, 200590UL, "mercury", {2, 1, 0}}, /* 80 */
    {"Tl", CAT_METAL, 162, 204380UL, "thallium", {1, 3, 0}}, /* 81 */
    {"Pb", CAT_METAL, 233, 207200UL, "lead", {2, 4, 0}}, /* 82 */
    {"Bi", CAT_METAL, 202, 208980UL, "bismuth", {3, 5, 0}}, /* 83 */
    {"Po", CAT_METAL, 200, 209000UL, "polonium", {2, 4, 0}}, /* 84 */
    {"At", CAT_METALLOID, 220, 210000UL, "astatine", {-1, 0, 0}}, /* 85 */
    {"Rn", CAT_NOBLE, 220, 222000UL, "radon", {0, 0, 0}}, /* 86 */
    {"Fr", CAT_METAL, 70, 223000UL, "francium", {1, 0, 0}}, /* 87 */
    {"Ra", CAT_METAL, 90, 226000UL, "radium", {2, 0, 0}}, /* 88 */
    {"Ac", CAT_METAL, 110, 227000UL, "actinium", {3, 0, 0}}, /* 89 */
    {"Th", CAT_METAL, 130, 232040UL, "thorium", {4, 0, 0}}, /* 90 */
    {"Pa", CAT_METAL, 150, 231040UL, "protactinium", {5, 0, 0}}, /* 91 */
    {"U", CAT_METAL, 138, 238030UL, "uranium", {6, 4, 0}}, /* 92 */
    {"Np", CAT_METAL, 136, 237000UL, "neptunium", {5, 0, 0}}, /* 93 */
    {"Pu", CAT_METAL, 128, 244000UL, "plutonium", {4, 0, 0}}, /* 94 */
    {"Am", CAT_METAL, 113, 243000UL, "americium", {3, 0, 0}}, /* 95 */
    {"Cm", CAT_METAL, 128, 247000UL, "curium", {3, 0, 0}}, /* 96 */
    {"Bk", CAT_METAL, 130, 247000UL, "berkelium", {3, 0, 0}}, /* 97 */
    {"Cf", CAT_METAL, 130, 251000UL, "californium", {3, 0, 0}}, /* 98 */
    {"Es", CAT_METAL, 130, 252000UL, "einsteinium", {3, 0, 0}}, /* 99 */
    {"Fm", CAT_METAL, 130, 257000UL, "fermium", {3, 0, 0}}, /* 100 */
    {"Md", CAT_METAL, 130, 258000UL, "mendelevium", {3, 0, 0}}, /* 101 */
    {"No", CAT_METAL, 130, 259000UL, "nobelium", {2, 0, 0}}, /* 102 */
    {"Lr", CAT_METAL, 130, 266000UL, "lawrencium", {3, 0, 0}}, /* 103 */
    {"Rf", CAT_METAL, 0, 267000UL, "rutherfordium", {0, 0, 0}}, /* 104 */
    {"Db", CAT_METAL, 0, 268000UL, "dubnium", {0, 0, 0}}, /* 105 */
    {"Sg", CAT_METAL, 0, 269000UL, "seaborgium", {0, 0, 0}}, /* 106 */
    {"Bh", CAT_METAL, 0, 270000UL, "bohrium", {0, 0, 0}}, /* 107 */
    {"Hs", CAT_METAL, 0, 277000UL, "hassium", {0, 0, 0}}, /* 108 */
    {"Mt", CAT_METAL, 0, 278000UL, "meitnerium", {0, 0, 0}}, /* 109 */
    {"Ds", CAT_METAL, 0, 281000UL, "darmstadtium", {0, 0, 0}}, /* 110 */
    {"Rg", CAT_METAL, 0, 282000UL, "roentgenium", {0, 0, 0}}, /* 111 */
    {"Cn", CAT_METAL, 0, 285000UL, "copernicium", {0, 0, 0}}, /* 112 */
    {"Nh", CAT_METAL, 0, 286000UL, "nihonium", {0, 0, 0}}, /* 113 */
    {"Fl", CAT_METAL, 0, 289000UL, "flerovium", {0, 0, 0}}, /* 114 */
    {"Mc", CAT_METAL, 0, 290000UL, "moscovium", {0, 0, 0}}, /* 115 */
    {"Lv", CAT_METAL, 0, 293000UL, "livermorium", {0, 0, 0}}, /* 116 */
    {"Ts", CAT_METALLOID, 0, 294000UL, "tennessine", {0, 0, 0}}, /* 117 */
    {"Og", CAT_NOBLE, 0, 294000UL, "oganesson", {0, 0, 0}}, /* 118 */
};

fp_t elem_mass(uint8_t z) {
    return (fp_t)elements[z].mass1000 / 1000;
}

static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

uint8_t elem_find(const char *sym, uint8_t len) {
    uint8_t z;
    if (len < 1 || len > 2) return 0;
    for (z = 1; z <= NUM_ELEMENTS; z++) {
        const char *s = elements[z].sym;
        if (lower(s[0]) == lower(sym[0]) &&
            ((len == 1 && s[1] == 0) || (len == 2 && s[1] && s[1] == lower(sym[1]))))
            return z;
    }
    return 0;
}

uint8_t elem_find_name(const char *name, uint8_t len) {
    uint8_t z, i;
    for (z = 1; z <= NUM_ELEMENTS; z++) {
        const char *s = elements[z].name;
        for (i = 0; i < len && s[i] && s[i] == lower(name[i]); i++) ;
        if (i == len && s[i] == 0) return z;
    }
    /* accepted alternate spellings */
    if (len == 9 && !memcmp(name, "aluminium", 9)) return 13;
    if (len == 7 && !memcmp(name, "sulphur", 7)) return 16;
    if (len == 6 && !memcmp(name, "caesium", 6)) return 55;
    return 0;
}

bool elem_is_metal(uint8_t z) {
    return elements[z].cat == CAT_METAL;
}

uint8_t elem_rarity(uint8_t z) {
    /* Elements a first-year chem student types. Tier decides auto-case
     * splits such as "baso4" -> BaSO4 (not BAsO4) and "nh3" -> NH3 (not Nh3). */
    static const uint8_t tier1[] = {5, 33, 45, 51, 72, 75, 76, 77, 0}; /* B As Rh Sb Hf Re Os Ir */
    uint8_t i;
    if (z >= 84 && z != 86 && z != 88 && z != 92) return 3;
    if (z >= 57 && z <= 71) return 2;
    if (z == 39 || z == 41 || z == 43 || z == 49 || z == 73 || z == 81) return 2;
    for (i = 0; tier1[i]; i++) if (tier1[i] == z) return 1;
    return 0;
}

/* ---- electron configuration ---- */
const uint8_t sub_n[NUM_SUBSHELLS] = {1,2,2,3,3,4,3,4,5,4,5,6,4,5,6,7,5,6,7};
const uint8_t sub_l[NUM_SUBSHELLS] = {0,0,1,0,1,0,2,1,0,2,1,0,3,2,1,0,3,2,1};
/* noble gases and how many aufbau subshells they fill */
static const uint8_t noble_z[] = {2, 10, 18, 36, 54, 86};
static const uint8_t noble_nsub[] = {1, 3, 5, 8, 11, 15};

/* anomalies: z, from subshell, to subshell, electrons moved */
static const uint8_t anomaly[][4] = {
    {24, 5, 6, 1}, {29, 5, 6, 1},                       /* Cr Cu: 4s -> 3d */
    {41, 8, 9, 1}, {42, 8, 9, 1}, {44, 8, 9, 1}, {45, 8, 9, 1},
    {46, 8, 9, 2}, {47, 8, 9, 1},                        /* Nb Mo Ru Rh Pd Ag */
    {78, 11, 13, 1}, {79, 11, 13, 1},                    /* Pt Au: 6s -> 5d */
    {57, 12, 13, 1}, {58, 12, 13, 1}, {64, 12, 13, 1},  /* La Ce Gd: 4f -> 5d */
    {89, 16, 17, 1}, {90, 16, 17, 2}, {91, 16, 17, 1},
    {92, 16, 17, 1}, {93, 16, 17, 1}, {96, 16, 17, 1},  /* Ac..Cm: 5f -> 6d */
};

static uint8_t cap(uint8_t i) { return (uint8_t)(4 * sub_l[i] + 2); }

bool econfig(uint8_t z, int8_t charge, econfig_t *c) {
    int e = (int)z - charge, left;
    uint8_t i, k;
    if (z < 1 || z > NUM_ELEMENTS || e < 0 || e > 118) return false;
    memset(c, 0, sizeof *c);
    /* neutral atom by aufbau + anomalies */
    left = z;
    for (i = 0; i < NUM_SUBSHELLS && left > 0; i++) {
        c->occ[i] = (uint8_t)(left < cap(i) ? left : cap(i));
        left -= c->occ[i];
    }
    for (k = 0; k < sizeof anomaly / sizeof anomaly[0]; k++) {
        if (anomaly[k][0] == z) {
            c->occ[anomaly[k][1]] -= anomaly[k][3];
            c->occ[anomaly[k][2]] += anomaly[k][3];
        }
    }
    if (charge > 0) {
        /* cations lose electrons from the highest n first (4s before 3d) */
        for (left = charge; left > 0; left--) {
            int best = -1;
            for (i = 0; i < NUM_SUBSHELLS; i++) {
                if (!c->occ[i]) continue;
                if (best < 0 || sub_n[i] > sub_n[best] ||
                    (sub_n[i] == sub_n[best] && sub_l[i] > sub_l[best])) best = i;
            }
            if (best < 0) return false;
            c->occ[best]--;
        }
    } else if (charge < 0) {
        /* anions gain electrons in aufbau order */
        for (left = -charge; left > 0; left--) {
            for (i = 0; i < NUM_SUBSHELLS && c->occ[i] == cap(i); i++) ;
            if (i == NUM_SUBSHELLS) return false;
            c->occ[i]++;
        }
    }
    c->electrons = (uint8_t)e;
    /* largest noble-gas core that is completely filled and smaller than e */
    for (k = 0; k < sizeof noble_z; k++) {
        bool full = noble_z[k] < e;
        for (i = 0; full && i < noble_nsub[k]; i++) if (c->occ[i] != cap(i)) full = false;
        if (full) { c->core = noble_z[k]; c->ncore = noble_nsub[k]; }
    }
    return true;
}

static char *put_num(char *p, uint8_t v) {
    if (v >= 10) *p++ = (char)('0' + v / 10);
    *p++ = (char)('0' + v % 10);
    return p;
}

uint8_t econfig_str(const econfig_t *c, bool full, char *buf) {
    static const char lch[] = "spdf";
    char *p = buf;
    uint8_t i = 0;
    if (!full && c->core) {
        *p++ = '[';
        *p++ = elements[c->core].sym[0];
        if (elements[c->core].sym[1]) *p++ = elements[c->core].sym[1];
        *p++ = ']';
        i = c->ncore;
    }
    for (; i < NUM_SUBSHELLS; i++) {
        if (!c->occ[i]) continue;
        if (p > buf && p[-1] != ']') *p++ = ' ';
        p = put_num(p, sub_n[i]);
        *p++ = lch[sub_l[i]];
        p = put_num(p, c->occ[i]);
    }
    if (p == buf) { *p++ = '0'; }
    *p = 0;
    return (uint8_t)(p - buf);
}

uint8_t econfig_dcount(const econfig_t *c) {
    uint8_t i, d = 0;
    for (i = c->ncore; i < NUM_SUBSHELLS; i++) if (sub_l[i] == 2) d += c->occ[i];
    return d;
}

/* ---- polyatomic ions ---- */
const polyion_t polyions[NUM_POLYIONS] = {
    {"NH4", 1, "ammonium"}, {"H3O", 1, "hydronium"}, {"Hg2", 2, "mercury(I)"},
    {"OH", -1, "hydroxide"}, {"CN", -1, "cyanide"}, {"NO3", -1, "nitrate"},
    {"NO2", -1, "nitrite"}, {"ClO4", -1, "perchlorate"}, {"ClO3", -1, "chlorate"},
    {"ClO2", -1, "chlorite"}, {"ClO", -1, "hypochlorite"}, {"BrO4", -1, "perbromate"},
    {"BrO3", -1, "bromate"}, {"IO4", -1, "periodate"}, {"IO3", -1, "iodate"},
    {"MnO4", -1, "permanganate"}, {"HCO3", -1, "hydrogen carbonate"},
    {"HSO4", -1, "hydrogen sulfate"}, {"H2PO4", -1, "dihydrogen phosphate"},
    {"C2H3O2", -1, "acetate"}, {"CH3CO2", -1, "acetate"}, {"SCN", -1, "thiocyanate"},
    {"CO3", -2, "carbonate"}, {"SO4", -2, "sulfate"}, {"SO3", -2, "sulfite"},
    {"SeO4", -2, "selenate"}, {"CrO4", -2, "chromate"}, {"Cr2O7", -2, "dichromate"},
    {"O2", -2, "peroxide"}, {"HPO4", -2, "hydrogen phosphate"}, {"C2O4", -2, "oxalate"},
    {"S2O3", -2, "thiosulfate"}, {"PO4", -3, "phosphate"}, {"PO3", -3, "phosphite"},
};

int polyion_find(const char *f, uint8_t len) {
    int i;
    for (i = 0; i < NUM_POLYIONS; i++)
        if (strlen(polyions[i].formula) == len && !memcmp(polyions[i].formula, f, len)) return i;
    return -1;
}

int polyion_find_name(const char *name, uint8_t len) {
    int i;
    for (i = 0; i < NUM_POLYIONS; i++)
        if (strlen(polyions[i].name) == len && !memcmp(polyions[i].name, name, len)) return i;
    return -1;
}
