/* units: TEST_MAP topic 2 rows, prefixes, powers, compounds, lone m */
#include "../src/common/units.h"
#include "test.h"

/* "2.45cm2>mm2": number + unit, STO arrow, unit; returns sci answer + unit */
static void conv(const char *text, const char *want) {
    const char *s = ti(text);
    qty_t q;
    unit_t to;
    uint8_t n = qty_parse(s, &q);
    fp_t v;
    char a[40], u[24], got[64];
    CHECK(n && s[n] == CH_STO, "%s: parse", text);
    if (!n || s[n] != CH_STO) return;
    CHECK(unit_parse(s + n + 1, &to) == strlen(s + n + 1), "%s: target", text);
    CHECK(unit_resolve_pair(&q.u, &to), "%s: dims", text);
    CHECK(unit_convert(q.n.v, &q.u, &to, &v), "%s: convert", text);
    num_fmt_sci(num_with_sf(v, q.n.sf), a);
    unit_str(&to, u);
    sprintf(got, "%s %s", ascii(a), ascii(u));
    CHECK_STR(got, want);
}

static void parse(const char *text, const char *disp, int used) {
    unit_t u;
    char b[32];
    int n = unit_parse(text, &u);
    unit_str(&u, b);
    CHECK(n == used, "%s used %d want %d", text, n, used);
    CHECK_STR(ascii(b), disp);
}

int main(void) {
    /* Topic 2, Day 1-7 + Prep */
    conv("2.45cm2>mm2", "2.45E2 mm2");
    conv(".0350m3>cm3", "3.50E4 cm3");
    conv("4.70km2>m2", "4.70E6 m2");
    conv("525cm2>m2", "5.25E-2 m2");
    conv("1.80cm3>mm3", "1.80E3 mm3");
    conv("10.0m2>ft2", "1.08E2 ft2");
    conv("2.50m3>l", "2.50E3 L");
    conv("3.19m2>cm2", "3.19E4 cm2");
    /* compound units, prefixes, energy, temperature */
    conv("3.5mi/hr>m/s", "1.6E0 m/s");
    conv("1.00ml>cm3", "1.00E0 cm3");
    conv("25.0c>k", "2.98E2 K");
    conv("98.6f>c", "3.70E1 oC");
    conv("1.00kcal>j", "4.18E3 J");
    conv("2.50ghz>hz", "2.50E9 Hz");
    conv("2.50mhz>khz", "2.50E3 kHz");
    conv("1.000mol>particles", "6.022E23 particles");
    conv("1.0atm>torr", "7.6E2 torr");
    /* lone m: molar when only molar works, else meter */
    conv(".640m>mol/l", "6.40E-1 mol/L");
    conv("1.50m>cm", "1.50E2 cm");
    {
        unit_t a, b;
        unit_parse("m", &a);
        CHECK(a.ambig == 1, "lone m ambiguous");
        unit_parse("m2", &a);
        CHECK(a.ambig == 0, "m2 not ambiguous");
        unit_parse("M", &a);
        CHECK(a.t[0].id == U_MOLAR && !a.ambig, "M molar");
        /* 250.0ml .640m -> mol : ml x m must be mol, so m is molar */
        unit_parse("ml", &a);
        unit_parse("m", &b);
        unit_mul(&a, &b, 1);
        {
            unit_t mol;
            unit_parse("mol", &mol);
            CHECK(unit_resolve_pair(&a, &mol), "ml*m -> mol");
            CHECK(a.t[1].id == U_MOLAR, "resolved molar");
        }
    }
    parse("mol", "mol", 3);
    parse("mmol", "mmol", 4);
    parse("min", "min", 3);
    parse("mi/hr", "mi/hr", 5);
    parse("g/mol", "g/mol", 5);
    parse("kg*m2/s2", "kg.m2/s2", 8);
    parse("nm", "nm", 2);
    parse("ul", "uL", 2);
    parse("moles", "mol", 5);
    parse("hrs", "hr", 3);
    parse("kj/mol", "kJ/mol", 6);
    parse("m/s^2", "m/s2", 5);
    parse("naoh", "", 0);
    parse("kcl", "", 0);
    TEST_END();
}
