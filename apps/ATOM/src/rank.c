/* rank.c - which ion forms the strongest ionic bond: "na+ mg2+ sr2+ rb+".
 * Coulomb's law: attraction ~ q1*q2 / r, so the higher charge wins first and
 * the smaller ion (closer to its partner) breaks ties, as for lattice energy.
 * A lone opposite-sign ion in the list is the partner: "... cl-". */
#include <string.h>
#include "atom.h"

#define RANK_MAX 8

uint16_t ion_r2(uint8_t z, int8_t q) {
    uint8_t i;
    for (i = 0; i < ion_radii_n; i++)
        if (ion_radii[i].z == z && ion_radii[i].q == q) return ion_radii[i].r2;
    return 0;
}

char *put_r2(char *p, uint16_t r2) {
    p = put_u(p, (uint16_t)(r2 / 2));
    return r2 & 1 ? put_s(p, ".5") : p;
}

typedef struct { uint8_t z; int8_t q; uint16_t r2; } ion_t;

static uint8_t absq(int8_t q) { return (uint8_t)(q < 0 ? -q : q); }

/* a before b: higher charge, then smaller radius (no data last) */
static bool stronger(const ion_t *a, const ion_t *b) {
    if (absq(a->q) != absq(b->q)) return absq(a->q) > absq(b->q);
    if (!a->r2 || !b->r2) return a->r2 && !b->r2;
    return a->r2 < b->r2;
}

bool ion_rank(const char *in, char *out, char *small, work_t *w) {
    ion_t ion[RANK_MAX], t;
    uint8_t n = 0, pos = 0, i, j, partner = RANK_MAX;
    char line[40], *p;
    const char *s = in;
    if (!strchr(in, ' ')) return false;
    while (*s) {
        uint8_t len = 0;
        int8_t q;
        while (*s == ' ') s++;
        if (!*s) break;
        while (s[len] && s[len] != ' ') len++;
        if (n == RANK_MAX) return false;
        ion[n].z = element_of(s, len, &q);
        if (!ion[n].z || !q) return false;
        ion[n].q = q;
        ion[n].r2 = ion_r2(ion[n].z, q);
        if (q > 0) pos++;
        n++;
        s += len;
    }
    if (n < 2) return false;
    /* one ion of the other sign = the partner, not ranked */
    if (pos && pos < n && (pos == 1 || pos == n - 1) && n > 2)
        for (i = 0; i < n; i++)
            if ((ion[i].q > 0) == (pos == 1)) partner = i;
    if (partner < RANK_MAX) {
        t = ion[partner];
        ion[partner] = ion[n - 1];
        ion[n - 1] = t;
        partner = n - 1;
        n--;
    }
    /* insertion sort, stable */
    for (i = 1; i < n; i++) {
        t = ion[i];
        for (j = i; j && stronger(&t, &ion[j - 1]); j--) ion[j] = ion[j - 1];
        ion[j] = t;
    }

    fmt(small, "%S%q strongest", ion[0].z, ion[0].q);
    p = out;
    for (i = 0; i < n; i++) {
        if (i) p = put_s(p, i % 4 ? " > " : " >\n");
        p = fmt(p, "%S%q", ion[i].z, ion[i].q);
    }

    if (partner < RANK_MAX) {
        t = ion[partner];
        wf(w, "Strongest with %S%q: %S%q", t.z, t.q, ion[0].z, ion[0].q);
    } else {
        wf(w, "Strongest bond: %S%q", ion[0].z, ion[0].q);
    }
    for (i = 0; i < n; i++) {
        p = fmt(line, "%u. %S%q  charge %u  ", i + 1, ion[i].z, ion[i].q, absq(ion[i].q));
        if (ion[i].r2) fmt(put_r2(p, ion[i].r2), " pm");
        else put_s(p, "r ?");
        work_add(w, line, NULL, NULL, NULL);
    }
    work_add(w, "Rule: higher charge first,", NULL, NULL, NULL);
    work_add(w, "then the smaller ion.", NULL, NULL, NULL);
    work_add(w, "Force ~ q1 q2 / r^2: more", NULL, NULL, NULL);
    work_add(w, "charge and closer ions =", NULL, NULL, NULL);
    work_add(w, "stronger pull, as for", NULL, NULL, NULL);
    work_add(w, "lattice energy.", NULL, NULL, NULL);
    work_add(w, "Ionic radii: Shannon,", NULL, NULL, NULL);
    work_add(w, "6-coordinate.", NULL, NULL, NULL);
    return true;
}
