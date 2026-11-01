/* ATOM: configs (topics 3-4), element card, isotopes, light, the isotope
 * form solver, key counts, and a smoke run of every field screen. */
#define main atom_main
#include "../apps/ATOM/src/main.c"
#undef main
#include "../apps/ATOM/src/card.c"
#include "../apps/ATOM/src/light.c"
#include "../apps/ATOM/src/iso.c"
#include "../apps/ATOM/src/table.c"
#include "../apps/ATOM/src/orbital.c"
#include "../apps/ATOM/src/atomdata.c"
#include "../src/common/entry.h"
#include "plat_mock.h"
#include "test.h"

static work_t w;
static char out[UI_OUT], small[UI_SMALL];

static void run(const char *in) {
    out[0] = small[0] = 0;
    work_clear(&w);
    compute(ti(in), out, small, &w);
}

static void want(const char *in, const char *o, const char *s) {
    run(in);
    CHECK_STR(ascii(out), o);
    if (s) CHECK_STR(ascii(small), s);
}

static bool work_has(const char *s) {
    uint8_t i;
    for (i = 0; i < w.n; i++) if (strstr(ascii(w.line[i]), s)) return true;
    return false;
}

static void iso_set(const char *const f[ISO_ROWS * 2 + 1]) {
    uint8_t i;
    for (i = 0; i < ISO_ROWS * 2 + 1; i++) strcpy(ast.iso[i], f[i] ? f[i] : "");
    solve();
}

static const char *fmtn(num_t n) {
    static char b[32];
    num_fmt_plain(n, b);
    return ascii(b);
}

/* keys that are not data, for a line typed in ATOM (smart lock) */
static int extra_keys(const char *line) {
    uint8_t extra = 0;
    char t[64], *o = t;
    const char *s;
    for (s = ti(line); *s; s++) *o++ = (*s == 'E' && ((s[1] >= '0' && s[1] <= '9') || s[1] == CH_NEG)) ? CH_EE : *s;
    *o = 0;
    return entry_plan(t, SMART_ALL, NULL, 0, &extra) ? extra : 99;
}

int main(void) {
    memset(&ast, 0, sizeof ast);
    /* topic 3: ion configs and d count */
    want("fe3+", "[Ar]3d5", "5 d e-");
    want("cu2+", "[Ar]3d9", "9 d e-");
    want("ni2+", "[Ar]3d8", "8 d e-");
    want("cr3+", "[Ar]3d3", "3 d e-");
    want("zn2+", "[Ar]3d10", "10 d e-");
    want("co3+", "[Ar]3d6", "6 d e-");
    want("ti2+", "[Ar]3d2", "2 d e-");
    want("co2+", "[Ar]3d7", "7 d e-");
    /* topic 4: atoms */
    want("cr", "[Ar]4s1 3d5", "Cr 52.0");
    want("cu", "[Ar]4s1 3d10", NULL);
    want("ag", "[Kr]5s1 4d10", NULL);
    want("fe", "[Ar]4s2 3d6", "Fe 55.85");
    want("ni", "[Ar]4s2 3d8", NULL);
    want("mn", "[Ar]4s2 3d5", NULL);
    want("29", "[Ar]4s1 3d10", "Cu 63.55");
    want("og", "[Rn]7s2 5f14 6d10 7p6", "Og [294]");
    want("mo", "[Kr]5s1 4d5", NULL);
    want("iron", "[Ar]4s2 3d6", NULL);
    want("co", "[Ar]4s2 3d7", NULL);              /* ATOM reads co as cobalt */
    /* element card */
    run("fe");
    CHECK(work_has("26 Fe iron"), "card name");
    CHECK(work_has("Mass 55.85 EN 1.83"), "card mass");
    CHECK(work_has("Group 8 Period 4 d-block"), "card group");
    CHECK(work_has("Transition metal"), "card family");
    CHECK(work_has("metal, solid at 25oC"), "card type state");
    CHECK(work_has("r 140pm cov.r 132pm") || work_has("r 140pm"), "card radii");
    CHECK(work_has("Most common: Fe-56") && work_has("26p 30n 26e"), "card isotope");
    CHECK(work_has("Shells 2 8 14 2"), "card shells");
    CHECK(work_has("4 unpaired: paramagnetic"), "card unpaired");
    CHECK(work_has("Ions Fe3+ Fe2+"), "card ions");
    CHECK(work_has("1s2 2s2 2p6 3s2 3p6 4s2"), "card full config");
    run("n");
    CHECK(work_has("Gains 3e- -> 3- anion"), "n rule");
    CHECK(work_has("Lewis.N."), "n lewis");
    CHECK(work_has("Valence e- 5 (n=2)"), "n valence");
    run("zn");
    CHECK(work_has("0 unpaired: diamagnetic"), "zn dia");
    run("og");
    CHECK(work_has("Noble gas") && work_has("unknown at 25"), "og state");
    CHECK(work_has("Most stable: Og-294"), "og isotope");
    run("tc");
    CHECK(work_has("Mass [98]"), "tc mass");
    for (uint8_t z = 1; z <= NUM_ELEMENTS; z++) {     /* every card fits */
        char s[8];
        sprintf(s, "%u", z);
        run(s);
        CHECK(w.n > 8 && w.n < WORK_LINES, "card %u lines %u", z, w.n);
        for (uint8_t i = 0; i < w.n; i++)
            CHECK(strlen(w.line[i]) <= WORK_COLS, "card %u line %u full: %s", z, i, ascii(w.line[i]));
    }
    CHECK(el_group(1) == 1 && el_group(2) == 18 && el_group(5) == 13 && el_group(26) == 8 &&
          el_group(57) == 0 && el_group(72) == 4 && el_group(86) == 18 && el_group(118) == 18, "groups");
    CHECK(el_block(2) == 's' && el_block(13) == 'p' && el_block(30) == 'd' && el_block(92) == 'f', "blocks");
    CHECK(el_family(1) == 5 && el_family(11) == 0 && el_family(35) == 6 && el_family(14) == 4 &&
          el_family(82) == 3 && el_family(63) == 8 && el_family(92) == 9, "families");
    CHECK(el_state(80) == 1 && el_state(35) == 1 && el_state(7) == 2 && el_state(26) == 0, "states");
    /* isotopes typed on the line */
    want("cl37", "17p 20n 17e", "Cl-37");
    want("37cl", "17p 20n 17e", NULL);
    want("c-14", "6p 8n 6e", "C-14");
    want("14c", "6p 8n 6e", NULL);
    want("h2", "1p 1n 1e", "H-2");
    want("24mg", "12p 12n 12e", NULL);
    want("40k", "19p 21n 19e", NULL);
    /* light: 12 nm -> E 1.66e-17 J, v 2.50e16 Hz (PHOTON check values) */
    want("12.0nm", "1.66E-17 J", "ultraviolet");
    want("1.00nm", "1.99E-16 J", "X-ray");
    want("12.0nm>hz", "2.50E16 Hz", NULL);
    want("12nm>j", "1.7E-17 J", NULL);
    want("5.00E14hz>nm", "600. nm", "orange");
    want("5.00E14hz", "3.31E-19 J", NULL);
    want("3.31E-19j>nm", "600. nm", NULL);
    want("3.31E-19j>hz", "5.00E14 Hz", NULL);
    want("500.nm>kj/mol", "239 kJ/mol", "green");
    want("239kj/mol>nm", "501 nm", NULL);
    want("2.0m", "9.9E-26 J", "radio");
    want("green", "495-570 nm", "visible");
    want("red", "620-750 nm", NULL);
    run("12.0nm");
    CHECK(work_has("E = hc/L = 1.66E-17 J"), "light work");
    ast.c_sel = 1;
    want("12.0nm>hz", "2.50E16 Hz", NULL);
    want("600.nm>hz", "5.00E14 Hz", NULL);
    ast.c_sel = 0;
    /* not ours: echoed */
    want("xyz", "xyz", "?");
    /* isotope form */
    {
        static const char *ag[] = {"106.905", "51.839", "108.905", 0, 0, 0, 0, 0, 0, 0, 0};
        static const char *cu[] = {"62.930", 0, "64.928", 0, 0, 0, 0, 0, 0, 0, "63.546"};
        static const char *mass[] = {0, "69.15", "64.928", "30.85", 0, 0, 0, 0, 0, 0, "63.546"};
        static const char *mg[] = {"23.985", "78.99", "24.986", "10.00", "25.983", "11.01", 0, 0, 0, 0, 0};
        static const char *frac[] = {"10.013", ".199", "11.009", ".801", 0, 0, 0, 0, 0, 0, 0};
        static const char *norm[] = {"10.013", "20", "11.009", "79", 0, 0, 0, 0, 0, 0, 0};
        static const char *bad[] = {"10.013", 0, "11.009", 0, 0, 0, 0, 0, 0, 0, 0};
        iso_set(ag);
        CHECK_STR(fmtn(val[3]), "48.161");
        CHECK_STR(fmtn(val[F_AVG]), "107.868");
        CHECK(have[3] == 2 && have[F_AVG] == 2 && !msg[0], "ag solved");
        iso_set(cu);
        CHECK_STR(fmtn(val[1]), "69.17");
        CHECK_STR(fmtn(val[3]), "30.83");
        iso_set(mass);
        CHECK(have[0] == 2 && FP_FABS(val[0].v - 62.93) < 0.01, "missing mass %s", fmtn(val[0]));
        iso_set(mg);
        CHECK_STR(fmtn(val[F_AVG]), "24.31");
        iso_set(frac);
        CHECK_STR(fmtn(val[F_AVG]), "10.81");
        iso_set(norm);
        CHECK(have[F_AVG] == 2 && strstr(msg, "normalized"), "normalize %s", msg);
        CHECK(FP_FABS(val[F_AVG].v - (10.013 * 20 + 11.009 * 79) / 99) < 0.01, "normalized value");
        iso_set(bad);
        CHECK(!strcmp(msg, "need avg mass") && have[F_AVG] == 0, "need avg %s", msg);
    }
    /* key counts: every ATOM line within the budget of 12 */
    {
        static const char *lines[] = {"fe3+", "cu2+", "cr", "29", "iron", "cl37", "c-14",
                                      "12nm>j", "12.0nm>hz", "5.00E14hz>nm", "3.31E~19j>nm",
                                      "500.nm>kj/mol", "green", "violet"};
        unsigned i;
        for (i = 0; i < sizeof lines / sizeof lines[0]; i++) {
            int k = extra_keys(lines[i]);
            printf("  ATOM %-14s %d keys + data\n", lines[i], k);
            CHECK(k <= 12, "%s: %d keys", lines[i], k);
        }
    }
    /* field screens run on the mock OS and return */
    {
        static const uint8_t keys[] = {
            K_YEQU, K_RIGHT, K_DOWN, K_DOWN, K_2, K_3, K_4, K_1, K_LEFT, K_UP, K_ENTER,   /* table: pick */
            K_GRAPH, K_CLEAR,                                                             /* card */
            K_WINDOW, K_1, K_0, K_DECPNT, K_5, K_ENTER, K_5, K_0, K_ENTER, K_1, K_1, K_ENTER,
            K_ENTER, K_GRAPH, K_CLEAR, K_DEL, K_CLEAR,                                   /* isotopes */
            K_ZOOM, K_RIGHT, K_RIGHT, K_4, K_0, K_LEFT, K_6, K_7, K_8, K_9, K_CLEAR,       /* orbitals */
            K_TRACE, K_2, K_CLEAR,                                                        /* light */
            K_2ND, K_MODE};
        mock_state_reset();
        memset(&ast, 0, sizeof ast);
        mock_keys(keys, sizeof keys);
        atom_main();
        CHECK(mock_taken == (int)sizeof keys, "field keys %d", mock_taken);
        CHECK(ast.c_sel == 1 && ast.tab == 8, "screens kept state c=%u tab=%u", ast.c_sel, ast.tab);
        CHECK_STR(app_entry()->last, "Ru");
        CHECK_STR(ast.iso[0], "10.5");
        CHECK(ast.irow == 2 && ast.iso[2][0] == '1', "iso cursor %u", ast.irow);
    }
    TEST_END();
}
