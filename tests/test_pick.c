/* test_pick: popup menus redraw only the rows that change. The same key
 * script runs twice, once as on the calculator and once redrawing every
 * row on every key; at every key poll both screens must be identical.
 * Covers a short OPTIONS menu and a long scrolling list. */
#include <stdlib.h>
#include "../src/common/ui.h"
#include "../src/common/plat.h"
#include "../src/common/keys.h"
#include "plat_mock.h"
#include "test.h"

#define FRAME (LCD_W * LCD_H)
static uint16_t *frames[2];
static int nframes[2], pass;

static void grab(void) {
    if (mock_polls != 1 || nframes[pass] >= 64) return;   /* first poll after a key */
    memcpy(frames[pass] + (size_t)nframes[pass] * FRAME, mock_fb, FRAME * 2);
    nframes[pass]++;
}

static void run(const char *title, const char *const *items, uint8_t n, const uint8_t *moves, int nm,
                const char *name) {
    uint8_t q[128];
    int i, qn = 0;
    for (i = 0; i < nm; i++) { q[qn++] = 0; q[qn++] = moves[i]; }
    q[qn++] = 0; q[qn++] = K_CLEAR;
    for (pass = 0; pass < 2; pass++) {
        /* a busy background, so a missed redraw cannot hide */
        for (i = 0; i < FRAME; i++) mock_fb[i] = (uint16_t)((unsigned)i * 2654435761u >> 7);
        ui_full_redraw = pass == 1;
        nframes[pass] = 0;
        mock_polls = 0;
        mock_on_poll = grab;
        mock_keys(q, qn);
        ui_pick_at(title, items, n, 2);
    }
    ui_full_redraw = false;
    mock_on_poll = NULL;
    CHECK(nframes[0] == nm + 1 && nframes[1] == nm + 1, "%s: %d/%d frames", name, nframes[0], nframes[1]);
    for (i = 0; i < nframes[0] && i < nframes[1]; i++)
        CHECK(!memcmp(frames[0] + (size_t)i * FRAME, frames[1] + (size_t)i * FRAME, FRAME * 2),
              "%s: frame %d differs from a full redraw", name, i);
}

int main(void) {
    static const char *const menu[] = {"Polyatomic ions", "Acids", "Common names", "Monatomic anions"};
    static const char *const list[] = {"CO2", "N2", "H2", "NaCl", "KCl", "HCl", "HNO3", "NaOH", "CH3COOH",
                                       "H2SO4", "NH3", "C6H12O6", "CaCO3", "H2O"};
    static const uint8_t m1[] = {K_DOWN, K_DOWN, K_UP, K_UP, K_UP, K_DOWN, K_UP, K_UP};
    static const uint8_t m2[] = {K_DOWN, K_DOWN, K_DOWN, K_DOWN, K_DOWN, K_DOWN, K_DOWN, K_DOWN, K_DOWN,
                                 K_DOWN, K_DOWN, K_DOWN, K_UP, K_UP, K_UP, K_UP, K_UP, K_UP, K_UP, K_UP,
                                 K_UP, K_UP, K_UP, K_UP, K_UP, K_DOWN};
    frames[0] = malloc((size_t)64 * FRAME * 2);
    frames[1] = malloc((size_t)64 * FRAME * 2);
    ui_begin();
    run("OPTIONS", menu, 4, m1, (int)sizeof m1, "menu");
    run("SUBSTANCE", list, 14, m2, (int)sizeof m2, "list");
    free(frames[0]);
    free(frames[1]);
    TEST_END();
}
