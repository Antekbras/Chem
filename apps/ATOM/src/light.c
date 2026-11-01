/* light.c - photon energy, frequency, wavelength, region and color.
 *   12nm        -> E in J (work: ν, λ, E per mol, region, color)
 *   12nm→hz  5.0E14hz→nm  3.6E-19j→nm  250kj/mol→nm
 *   green       -> 495-570 nm (any of red orange yellow green blue indigo violet)
 * E = hν = hc/λ, ν = c/λ. c is the [TRACE] choice. */
#include <string.h>
#include "atom.h"

#define H_PLANCK 6.626e-34
#define N_A 6.022e23

const char *const c_text[3] = {"2.998\x1B" "8", "3.00\x1B" "8", "2.99792458\x1B" "8"};
static const fp_t c_val[3] = {2.998e8, 3.00e8, 2.99792458e8};

enum { Q_NONE, Q_LEN, Q_FREQ, Q_ENERGY, Q_EMOL };

static uint8_t kind(const unit_t *u, udim_t *d) {
    static const int8_t want[4][NDIM] = {
        {1, 0, 0, 0, 0}, {0, 0, -1, 0, 0}, {2, 1, -2, 0, 0}, {2, 1, -2, -1, 0}};
    uint8_t k;
    unit_dim(u, d);
    if (unit_is_empty(u)) return Q_NONE;
    for (k = 0; k < 4; k++) if (!memcmp(d->d, want[k], NDIM)) return (uint8_t)(k + 1);
    return Q_NONE;
}

/* visible colors, nm: violet .. red */
static const char *const colors[] = {"violet", "indigo", "blue", "green", "yellow", "orange", "red"};
static const uint16_t color_lo[] = {400, 425, 450, 495, 570, 590, 620, 750};

static const char *region(fp_t lam, const char **color) {
    uint8_t i;
    *color = NULL;
    if (lam < 1e-11) return "gamma ray";
    if (lam < 1e-8) return "X-ray";
    if (lam < 400e-9) return "ultraviolet";
    if (lam <= 750e-9) {
        for (i = 0; i < 6 && lam * 1e9 >= color_lo[i + 1]; i++) ;
        *color = colors[i];
        return "visible";
    }
    if (lam < 1e-3) return "infrared";
    if (lam < 1) return "microwave";
    return "radio";
}

static int8_t sf;                   /* sig figs of the given value */

static void add_num(work_t *w, const char *a, fp_t v, const char *unit) {
    char t[32], alt[32];
    num_fmt_answer(num_with_sf(v, sf), t, alt);
    work_add(w, a, t, unit, NULL);
}

static bool color_name(const char *in, char *out, char *small, work_t *w) {
    uint8_t i;
    char *p;
    for (i = 0; i < 7; i++) if (!strcmp(in, colors[i])) break;
    if (i == 7) return false;
    p = put_u(out, color_lo[i]);
    *p++ = '-';
    p = put_u(p, color_lo[i + 1]);
    put_s(p, " nm");
    strcpy(small, "visible");
    work_add(w, colors[i], ": ", out, NULL);
    {
        fp_t lo = c_val[ast.c_sel] / (color_lo[i + 1] * 1e-9), hi = c_val[ast.c_sel] / (color_lo[i] * 1e-9);
        sf = 3;
        add_num(w, "v from ", lo, " Hz");
        add_num(w, "  to ", hi, " Hz");
        add_num(w, "E from ", H_PLANCK * lo, " J");
        add_num(w, "  to ", H_PLANCK * hi, " J");
    }
    work_add(w, "longer wavelength =", NULL, NULL, NULL);
    work_add(w, "lower frequency and energy", NULL, NULL, NULL);
    return true;
}

bool light(const char *in, char *out, char *small, work_t *w) {
    qty_t q;
    unit_t to;
    udim_t d, dt;
    uint8_t n, k, kt;
    fp_t lam, nu, e, c = c_val[ast.c_sel], r, v;
    const char *reg, *col;
    if (color_name(in, out, small, w)) return true;
    n = qty_parse(in, &q);
    if (!n) return false;
    unit_resolve_m(&q.u, false);
    k = kind(&q.u, &d);
    if (k == Q_NONE) return false;
    memset(&to, 0, sizeof to);
    if (in[n] == CH_STO) {
        uint8_t m = unit_parse(in + n + 1, &to);
        if (!m || in[n + 1 + m]) return false;
        unit_resolve_m(&to, false);
    } else if (in[n]) {
        return false;
    }
    /* h, c and NA are constants: every result keeps the given sig figs */
    sf = q.n.sf;
    v = q.n.v * d.f;                                /* to SI: m, Hz, J */
    if (k == Q_EMOL) { v /= N_A; k = Q_ENERGY; }
    work_add(w, "L=wavelength v=frequency", NULL, NULL, NULL);
    work_add(w, "c = ", c_text[ast.c_sel], " m/s", NULL);
    work_add(w, "h = 6.626\x1B\x1A" "34 J s", NULL, NULL, NULL);
    if (k == Q_LEN) {
        lam = v;
        nu = c / lam;
        e = H_PLANCK * nu;
        add_num(w, "given L = ", lam, " m");
        add_num(w, "v = c/L = ", nu, " Hz");
        add_num(w, "E = hc/L = ", e, " J");
    } else if (k == Q_FREQ) {
        nu = v;
        lam = c / nu;
        e = H_PLANCK * nu;
        add_num(w, "given v = ", nu, " Hz");
        add_num(w, "L = c/v = ", lam, " m");
        add_num(w, "E = hv = ", e, " J");
    } else {
        e = v;
        nu = e / H_PLANCK;
        lam = c / nu;
        add_num(w, "given E = ", e, " J");
        add_num(w, "v = E/h = ", nu, " Hz");
        add_num(w, "L = c/v = ", lam, " m");
    }
    add_num(w, "L = ", lam * 1e9, " nm");
    add_num(w, "E x NA = ", e * (N_A / 1000), " kJ/mol");
    reg = region(lam, &col);
    work_add(w, reg, col ? ": " : NULL, col, NULL);
    /* the answer: wanted unit, else E in J (or λ in nm when E was given) */
    if (unit_is_empty(&to)) unit_parse(k == Q_ENERGY ? "nm" : "j", &to);
    kt = kind(&to, &dt);
    if (kt == Q_LEN) r = lam;
    else if (kt == Q_FREQ) r = nu;
    else if (kt == Q_ENERGY) r = e;
    else if (kt == Q_EMOL) r = e * N_A;
    else return false;
    {
        char main_[32], alt[32];
        uint8_t len;
        num_fmt_answer(num_with_sf(r / dt.f, sf), main_, alt);
        len = (uint8_t)strlen(main_);
        main_[len++] = ' ';
        unit_str(&to, main_ + len);
        strncpy(out, main_, UI_OUT - 1);
        out[UI_OUT - 1] = 0;
        strcpy(small, col ? col : reg);
    }
    return true;
}

void light_screen(void) {
    for (;;) {
        work_t w;
        uint8_t i, k;
        char t[WORK_COLS + 1], *p;
        work_clear(&w);
        for (i = 0; i < 3; i++) {
            p = t;
            *p++ = (char)('1' + i);
            *p++ = ':';
            p = put_s(p, i == ast.c_sel ? "\x05" : " ");
            p = put_s(p, "c=");
            put_s(p, c_text[i]);
            work_add(&w, t, " m/s", NULL, NULL);
        }
        work_add(&w, "E=hv=hc/L  v=c/L  c=Lv", NULL, NULL, NULL);
        work_add(&w, "gamma <10pm X-ray <10nm", NULL, NULL, NULL);
        work_add(&w, "UV <400nm  visible 400-750", NULL, NULL, NULL);
        work_add(&w, "IR <1mm micro <1m radio", NULL, NULL, NULL);
        for (i = 0; i < 7; i++) {
            p = put_s(t, colors[i]);
            while (p < t + 8) *p++ = ' ';
            p = put_u(p, color_lo[i]);
            *p++ = '-';
            p = put_u(p, color_lo[i + 1]);
            put_s(p, " nm");
            work_add(&w, t, NULL, NULL, NULL);
        }
        k = ui_show_work("LIGHT  1-3: pick c", &w);
        if (k == K_1) ast.c_sel = 0;
        else if (k == K_2) ast.c_sel = 1;
        else if (k == K_3) ast.c_sel = 2;
        else return;
    }
}
