/* entry editor + key counts for every TEST_MAP row (smart lock).
 * Each row is planned, replayed through entry_key(), and must type exactly
 * the text with no more non-data keys than the TEST_MAP smart column. */
#include "../src/common/entry.h"
#include "../src/common/fp.h"
#include "test.h"

typedef struct { const char *app, *q, *text; int data, smart; } row_t;

/* text: '>' = [STO→], 'E' after a digit = 2nd+[,] exponent, " ; " = next line */
static const row_t rows[] = {
    /* SIGFIG (topic-1 expressions from the Day 1-7 and Prep sheets) */
    {"SIGFIG", "1 D1", "(4.82*3.0100)/(15.2-2.11)", 25, 1},
    {"SIGFIG", "1 D2", "(8.314*2.0)/(0.500+12.31)", 25, 1},
    {"SIGFIG", "1 D3", "(25.0+1.236)*0.0402", 19, 1},
    {"SIGFIG", "1 D4", "(102.5-99.8)/(3.141*2.00)", 25, 1},
    {"SIGFIG", "1 D5", "6.02*10.00+1.3", 14, 1},
    {"SIGFIG", "1 D6", ".004050*12.0/3.7", 16, 1},
    {"SIGFIG", "1 D7", "(14.2+0.31-1.0)/2.500", 21, 1},
    {"SIGFIG", "1 P", "6.0509*5.000/(9.8+4.70)", 23, 1},
    /* CONVERT */
    {"CONVERT", "2 D1", "2.45cm2>mm2", 10, 6},
    {"CONVERT", "2 D2", ".0350m3>cm3", 10, 3},
    {"CONVERT", "2 D3", "4.70km2>m2", 9, 6},
    {"CONVERT", "2 D4", "525cm2>m2", 8, 6},
    {"CONVERT", "2 D5", "1.80cm3>mm3", 10, 3},
    {"CONVERT", "2 D6", "10.0m2>ft2", 9, 6},
    {"CONVERT", "2 D7", "2.50m3>l", 7, 3},
    {"CONVERT", "2 P", "3.19m2>cm2", 9, 6},
    {"CONVERT", "15 D1", "250.0ml .640m>mol", 16, 5},
    {"CONVERT", "15 D2", ".0500mol .250m>ml", 16, 5},
    {"CONVERT", "15 D3", ".0450mol 300.0ml>m", 17, 5},
    {"CONVERT", "15 D4", "250.0ml .400m nacl>g", 19, 5},
    {"CONVERT", "15 D5", "3.00m 25.0ml>150.0ml", 19, 7},
    {"CONVERT", "15 D6", "75.0ml 2.20m>mol", 15, 5},
    {"CONVERT", "15 D7", "4.00g naoh 500.0ml>m", 19, 5},
    {"CONVERT", "15 P", "160.0ml 1.85m>mol", 16, 5},
    {"CONVERT", "F13", "50.0ml .0100m h2so4>g", 20, 8},
    {"CONVERT", "F14", "2.23g c12h22o11 20.0ml>m", 23, 5},
    {"CONVERT", "F17", "pcl5>p", 5, 5},
    {"CONVERT", "F18", "10.00g n2o4>g n", 14, 4},
    {"CONVERT", "S11", "100.0ml .100m naoh>g", 19, 5},
    {"CONVERT", "S12", ".100m>50.0ml .0500m", 18, 7},
    {"CONVERT", "S20", "5.23E24 co2>g", 12, 5},
    {"CONVERT", "S22", "fe(no3)2>fe", 10, 4},
    {"CONVERT", "S24", "100.0g hno3>g o", 14, 3},
    {"CONVERT", "S25", "100.0g h2so4>mol", 15, 5},
    /* ATOM */
    {"ATOM", "3 D1", "fe3+", 4, 1}, {"ATOM", "3 D2", "cu2+", 4, 3},
    {"ATOM", "3 D3", "ni2+", 4, 1}, {"ATOM", "3 D4", "cr3+", 4, 2},
    {"ATOM", "3 D5", "zn2+", 4, 3}, {"ATOM", "3 D6", "co3+", 4, 2},
    {"ATOM", "3 D7", "ti2+", 4, 3}, {"ATOM", "3 P", "co2+", 4, 3},
    {"ATOM", "4 D1", "cr", 2, 2}, {"ATOM", "4 D2", "cu", 2, 2},
    {"ATOM", "4 D3", "ag", 2, 1}, {"ATOM", "4 D4", "fe", 2, 1},
    {"ATOM", "4 D5", "ni", 2, 1}, {"ATOM", "4 D6", "mn", 2, 2},
    {"ATOM", "4 D7", "29", 2, 1}, {"ATOM", "4 P", "mo", 2, 2},
    /* NAMES */
    {"NAMES", "5 D1", "tio2", 4, 3}, {"NAMES", "5 D2", "pbo2", 4, 3},
    {"NAMES", "5 D3", "febr3", 5, 2}, {"NAMES", "5 D4", "sncl4", 5, 3},
    {"NAMES", "5 D5", "cu2o", 4, 4}, {"NAMES", "5 D6", "v2o5", 4, 5},
    {"NAMES", "5 D7", "mn2o7", 5, 5}, {"NAMES", "5 P", "mns3", 4, 2},
    {"NAMES", "7 D1", "n2o4", 4, 3}, {"NAMES", "7 D2", "p4o10", 5, 5},
    {"NAMES", "7 D3", "sf6", 3, 1},
    {"NAMES", "7 D4", "diphosphorus pentasulfide", 25, 2},
    {"NAMES", "7 D5", "co", 2, 2}, {"NAMES", "7 D6", "cl2o7", 5, 5},
    {"NAMES", "7 D7", "disulfur dichloride", 19, 2},
    {"NAMES", "7 P", "s4cl2", 5, 3},
    {"NAMES", "9 D1", "ccl4 ; cabr2 ; (nh4)2so4", 18, 9},
    {"NAMES", "12 D1", "pcl3", 4, 2}, {"NAMES", "16 D1", "baso4", 5, 3},
    {"NAMES", "19 D1", "c12h22o11", 9, 3}, {"NAMES", "F11", "k2so4", 5, 5},
    /* TEST_MAP says 2, but x is ALPHA+[STO→] (design call) */
    {"NAMES", "S4", "iron3 oxide", 11, 3}, {"NAMES", "S5", "nitrous acid", 12, 2},
    {"NAMES", "S6", "h3p", 3, 2}, {"NAMES", "S7", "cus", 3, 2},
    {"NAMES", "S8", "clo3", 4, 2}, {"NAMES", "S15", "hf", 2, 1},
    {"NAMES", "S16", "nh3", 3, 1},
    /* REACT */
    {"REACT", "21 D1", "c3h8+o2>co2+h2o", 14, 8},
    {"REACT", "21 D2", "p4+o2>p4o10", 10, 11},
    {"REACT", "21 D3", "al+o2>al2o3", 10, 8},
    {"REACT", "21 D4", "c4h10+o2>co2+h2o", 15, 8},
    {"REACT", "21 D5", "fe+cl2>fecl3", 11, 6},
    {"REACT", "21 D6", "nh3+o2>no+h2o", 12, 8},
    {"REACT", "21 D7", "s8+o2>so2", 8, 7},
    {"REACT", "21 P", "s8+o2>so3", 8, 6},
    {"REACT", "22 D1", "n2o5>no2+o2", 10, 8},
    {"REACT", "22 D2", "al+cl2>alcl3", 11, 5},
    {"REACT", "22 D3", "b2h6+o2>b2o3+h2o", 15, 7},
    {"REACT", "22 D4", "p4o10+h2o>h3po4", 14, 8},
    {"REACT", "22 D5", "no+o2>no2", 8, 6},
    {"REACT", "22 D6", "cl2o7+h2o>hclo4", 14, 8},
    {"REACT", "22 D7", "fe2o3+co>fe+co2", 14, 4},
    {"REACT", "22 P", "pbr3+f2>p4f5+br2", 15, 8},
    {"REACT", "23 D1", "zn+hcl>zncl2+h2", 14, 4},
    {"REACT", "24 D1", "cu+hcl", 6, 2},
    {"REACT", "25 D1", "c3h8+o2", 7, 3},
    {"REACT", "F2", "h2+n2>nh3", 8, 2},
    {"REACT", "F4", "c8h18+o2", 8, 3},
    {"REACT", "F5", "fe+h2so4", 8, 3},
    {"REACT", "F6", "sn+al(no3)3", 11, 5},
    {"REACT", "F7", "hno3+koh", 8, 2},
    {"REACT", "F8", "na2s+cuno3", 10, 2},
    {"REACT", "F9", "kclo4+nh4f", 10, 3},
    {"REACT", "F12", "kcl+naoh", 8, 2},
    {"REACT", "F15", "k2o+al(no3)3>al2o3+kno3", 22, 11},
    {"REACT", "F16", "n2+h2o>n2o4+n2h4", 15, 7},
    {"REACT", "F19", "4mol b+6mol cl2>bcl3", 19, 7},
    {"REACT", "F21", "163kg ch4+o2>co2+h2o>mol o2", 25, 11},
    {"REACT", "F22", "12.0mol n2+30.0mol h2>nh3", 24, 6},
    {"REACT", "F23", "150.0g cuo+nh3>cu+n2+h2o>g n2", 27, 7},
    {"REACT", "F24", "35.5g kclo3>kcl+o2>g o2 ; *.835", 26, 8},
    {"REACT", "F25", "100.0g c7h6o+c2h4o>c9h8o+h2o>100.0g c9h8o", 39, 12},
    {"REACT", "S9", "h2co3+naoh>na2co3+h2o", 20, 7},
    {"REACT", "S10", "h2+n2>nh3", 8, 2},
    {"REACT", "S13", "fe+pb(no3)2", 11, 5},
    {"REACT", "S17", "naoh+h3po4", 10, 3},
    {"REACT", "S18", "agno3+nacl", 10, 2},
    {"REACT", "S19", "c8h18+o2", 8, 3},
    {"REACT", "S21", "al+hcl ; ni+hcl", 12, 4},
};

/* readable row text -> TI chars */
static void conv(const char *s, char *o) {
    for (; *s; s++) {
        if (*s == '>') *o++ = CH_STO;
        else if (*s == 'E' && s[1] >= '0' && s[1] <= '9') *o++ = CH_EE;
        else *o++ = *s;
    }
    *o = 0;
}

static int data_chars(const char *t) {
    int n = 0;
    for (; *t; t++) {
        if (*t == CH_STO) continue;
        if (!strncmp(t, " ; ", 3)) { t += 2; continue; }
        n++;
    }
    return n;
}

int main(void) {
    unsigned i;
    int better = 0;
    FILE *rep = fopen("keycounts.txt", "w");
    if (rep) fprintf(rep, "App      Q      keys(ours/TEST_MAP smart)  text\n");
    for (i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        const row_t *r = &rows[i];
        uint32_t smart = !strcmp(r->app, "SIGFIG") ? SMART_CALC : SMART_ALL;
        char text[128];
        uint8_t keys[200], extra = 0, n, k;
        entry_t e;
        const char *line;
        conv(r->text, text);
        n = entry_plan(text, smart, keys, sizeof keys, &extra);
        CHECK(n > 0, "%s %s: not typeable", r->app, r->q);
        if (!n) continue;
        CHECK(data_chars(text) == r->data, "%s %s: data %d want %d", r->app, r->q, data_chars(text), r->data);
        CHECK(extra <= r->smart, "%s %s: %d keys > smart %d", r->app, r->q, extra, r->smart);
        if (extra < r->smart) better++;
        if (rep) fprintf(rep, "%-8s %-6s %2d/%-2d %s\n", r->app, r->q, extra, r->smart, r->text);
        /* replay through the real editor */
        entry_init(&e, smart, LOCK_NUM);
        line = text;
        for (k = 0; k < n; k++) {
            uint8_t res = entry_key(&e, keys[k]);
            if (res == EK_ENTER) {
                const char *sep = strstr(line, " ; ");
                size_t len = sep ? (size_t)(sep - line) : strlen(line);
                char want[128];
                if (*line == '*' || *line == '+' || *line == '/') { strcpy(want, "Ans"); strncat(want, line, len); }
                else { memcpy(want, line, len); want[len] = 0; }
                CHECK(!strcmp(e.buf, want), "%s %s: typed \"%s\" want \"%s\"", r->app, r->q, ascii(e.buf), ascii(want));
                entry_done(&e);
                line = sep ? sep + 3 : line + len;
            }
        }
    }
    if (rep) fclose(rep);
    printf("key counts: %d of %u rows need fewer keys than the TEST_MAP smart column\n",
           better, (unsigned)(sizeof rows / sizeof rows[0]));

    /* ---- editor behaviour ---- */
    {
        entry_t e;
        entry_init(&e, SMART_ALL, LOCK_NUM);
        /* number-lock smart letters, letter-lock, either-lock keys */
        entry_key(&e, K_COS); entry_key(&e, K_SIN); entry_key(&e, K_3); entry_key(&e, K_ADD);
        CHECK_STR(e.buf, "fe3+");
        entry_key(&e, K_CLEAR);
        CHECK(e.len == 0, "clear");
        CHECK(entry_key(&e, K_CLEAR) == EK_CLEAR_EMPTY, "clear empty");
        entry_key(&e, K_UP);
        CHECK(e.lock == LOCK_LET, "lock toggle");
        entry_key(&e, K_LOG); entry_key(&e, K_7); entry_key(&e, K_0); entry_key(&e, K_9);
        CHECK_STR(e.buf, "no 9");
        entry_key(&e, K_ALPHA); entry_key(&e, K_STO);
        CHECK_STR(e.buf, "no 9x");
        entry_key(&e, K_ALPHA); entry_key(&e, K_COMMA);
        CHECK_STR(e.buf, "no 9xj");
        entry_key(&e, K_STO);
        CHECK(e.buf[6] == CH_STO, "sto arrow in letter-lock");
        /* cursor, overwrite, insert, delete */
        entry_set(&e, "kcl");
        entry_key(&e, K_LEFT); entry_key(&e, K_LEFT);
        e.lock = LOCK_LET;
        entry_key(&e, K_MATH);
        CHECK_STR(e.buf, "kal");
        entry_key(&e, K_2ND); entry_key(&e, K_DEL);
        CHECK(e.ins, "ins");
        entry_key(&e, K_APPS);
        CHECK_STR(e.buf, "kabl");
        entry_key(&e, K_DEL);
        CHECK_STR(e.buf, "kab");
        entry_key(&e, K_2ND); entry_key(&e, K_LEFT);
        CHECK(e.cur == 0, "2nd left");
        /* 2nd+, = EE ; SIGFIG keeps ^ and x² */
        entry_init(&e, SMART_CALC, LOCK_NUM);
        entry_key(&e, K_2); entry_key(&e, K_2ND); entry_key(&e, K_COMMA); entry_key(&e, K_3);
        entry_key(&e, K_POWER); entry_key(&e, K_SQUARE);
        CHECK(!strcmp(e.buf, "2\x1B" "3^\x12"), "EE ^ x2");
        /* ENTER keeps the line; a typed char starts a new line; operator = Ans */
        CHECK(entry_key(&e, K_ENTER) == EK_ENTER, "enter");
        entry_done(&e);
        CHECK(entry_key(&e, K_MUL) == EK_NEWLINE, "new line");
        CHECK_STR(e.buf, "Ans*");
        entry_done(&e);
        CHECK(entry_key(&e, K_5) == EK_NEWLINE && !strcmp(e.buf, "5"), "digit new line");
        entry_done(&e);
        CHECK(entry_key(&e, K_LEFT) == EK_CHANGED && !e.done, "edit after enter");
        entry_key(&e, K_2ND);
        CHECK(entry_key(&e, K_ENTER) == EK_CHANGED && !strcmp(e.buf, "5"), "2nd enter recall");
        entry_key(&e, K_2ND);
        CHECK(entry_key(&e, K_MODE) == EK_QUIT, "quit");
        CHECK(entry_key(&e, K_YEQU) == EK_FKEY && e.fkey == 0, "y=");
        CHECK(entry_key(&e, K_TRACE) == EK_FKEY && e.fkey == 3, "trace");
        CHECK(entry_key(&e, K_GRAPH) == EK_GRAPH, "graph");
        CHECK(entry_key(&e, K_VARS) == EK_OTHER, "other");
    }
    TEST_END();
}
