/* test_render: pixel-level check of the home-screen drawing.
 *
 * A fake OS (plat_mock.c) paints home-screen cells like the calculator:
 * one fixed box per cell, black on white. SIGFIG runs on it for real
 * (app_run with scripted keys). At every key poll the live screen must
 * equal an expected frame built from scratch: status bar, the text of
 * ui_layout drawn with the fake OS in its own cells, the small grey
 * decimal, and the cursor box when it is in its "on" phase. So any
 * leftover pixel (cursor boxes, old characters, old small text), any
 * misalignment with the OS grid, or a broken blink fails the test. Runs
 * on several OS geometries, including one with gaps between cells. */
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define main sigfig_main
#include "../apps/SIGFIG/src/main.c"
#undef main
#include "../src/common/app.h"
#include "../src/common/plat.h"
#include "plat_mock.h"
#include "test.h"

static uint16_t want[LCD_W * LCD_H];
static const app_def_t *app;
static int frames, mism;
static bool cursor_seen_on, cursor_seen_off;

enum { EXP_HOME, EXP_WORK };
static int expect = EXP_HOME, work_top;

/* ---- expected frames, built only from the fake OS ---- */

static void exp_invert(uint8_t r, uint8_t c) {
    int x0 = mock_os.ox + c * mock_os.pw, y0 = mock_os.oy + r * mock_os.ph, x, y;
    for (y = y0; y < y0 + mock_os.bh; y++)
        for (x = x0; x < x0 + mock_os.bw; x++) want[y * LCD_W + x] ^= 0xFFFF;
}

static void exp_recolor_row(uint8_t r, uint16_t fg, uint16_t bg) {
    int x, y, y0 = mock_os.oy + r * mock_os.ph;
    for (y = y0; y < y0 + mock_os.bh; y++)
        for (x = mock_os.ox; x < mock_os.ox + 26 * mock_os.pw; x++)
            want[y * LCD_W + x] = want[y * LCD_W + x] == 0xFFFF ? bg : fg;
}

static void exp_home(bool cursor_on) {
    srow_t r[UI_ROWS];
    uint8_t n, i, c, crow, ccol;
    const entry_t *e = app_entry();
    mock_target(want);
    plat_home_reset();
    n = ui_layout(app_screen(), e, UI_ROWS, r, &crow, &ccol);
    for (i = 0; i < n; i++) {
        for (c = 0; c < r[i].len; c++) mock_cell(i, (uint8_t)(r[i].col + c), r[i].text[c]);
        if (r[i].kind == ROW_OUT && r[i].small && r[i].small[0] &&
            mock_small_width(r[i].small) + 4 <= r[i].col * mock_os.pw && mock_os.sh <= mock_os.bh)
            plat_small(r[i].small, (uint16_t)(mock_os.ox + 1),
                       (uint8_t)(mock_os.oy + i * mock_os.ph + mock_os.bh - mock_os.sh), 0x7BEF, 0xFFFF);
    }
    if (cursor_on && crow != 0xFF) {
        mock_cell(crow, ccol, ui_cursor_glyph(e));
        exp_invert(crow, ccol);
    }
    mock_target(NULL);
}

static void exp_work(void) {
    uint8_t i, c;
    mock_target(want);
    plat_home_reset();
    for (c = 0; c < 26; c++) mock_cell(0, c, c < strlen(app->title) ? app->title[c] : ' ');
    exp_recolor_row(0, 0xFFFF, 0xD304);
    for (i = 1; i < UI_ROWS; i++) {
        int j = work_top + i - 1;
        const char *t = j < app_work()->n ? app_work()->line[j] : "";
        for (c = 0; c < 26 && c < strlen(t); c++) mock_cell(i, c, t[c]);
    }
    mock_target(NULL);
}

static void dump_diff(const char *what) {
    int x, y, n = 0;
    for (y = 0; y < LCD_H; y++)
        for (x = 0; x < LCD_W; x++)
            if (mock_fb[y * LCD_W + x] != want[y * LCD_W + x] && n++ < 3)
                printf("  %s: pixel (%d,%d) got %04X want %04X\n", what, x, y,
                       mock_fb[y * LCD_W + x], want[y * LCD_W + x]);
    printf("  %d pixels differ\n", n);
}

static int small_frames;
static void compare(const char *what) {
    int i;
    frames++;
    for (i = 0; i < LCD_W * LCD_H; i++)
        if (want[i] == 0x7BEF) { small_frames++; break; }
    if (memcmp(mock_fb, want, sizeof want)) {
        if (mism++ < 4) {
            printf("FAIL frame %d (%s) line \"%s\"\n", frames, what, ascii(app_entry()->buf));
            dump_diff(what);
        }
        t_fail++;
    } else t_pass++;
}

/* every poll: polls 1-2 after a key show the cursor on, 3-4 off, 5-6 on.. */
static int work_shown, taken, max_scroll_seen;
static void on_poll(void) {
    if (mock_taken != taken) {       /* a key was taken since the last poll */
        uint8_t k = mock_last_key;
        taken = mock_taken;
        if (expect == EXP_HOME) {
            if (k == K_GRAPH && app_work()->n) {
                expect = EXP_WORK;
                work_top = 0;
                work_shown++;
            }
        } else if (k == K_DOWN) {
            if (work_top + UI_ROWS - 1 < app_work()->n) work_top++;
        } else if (k == K_UP) {
            if (work_top) work_top--;
        } else expect = EXP_HOME;
    }
    if (expect == EXP_WORK) {
        exp_work(); compare("show work"); return; }
    {
        bool on = ((mock_polls - 1) / 2) % 2 == 0;
        uint8_t mx = ui_max_scroll(app_screen(), app_entry(), UI_ROWS);
        if (app_screen()->scroll > max_scroll_seen) max_scroll_seen = app_screen()->scroll;
        CHECK(app_screen()->scroll <= mx, "scroll %u past top %u", app_screen()->scroll, mx);
        exp_home(on);
        if (on) cursor_seen_on = true; else cursor_seen_off = true;
        compare(on ? "cursor on" : "cursor off");
    }
}

/* ---- scripts ---- */

static uint8_t q[4096];
static int qn;

static void key(uint8_t k) { q[qn++] = 0; q[qn++] = 0; q[qn++] = 0; q[qn++] = 0; q[qn++] = k; }
static void quick(uint8_t k) { q[qn++] = k; }
static void type(const char *text) {
    char t[128], *o = t;
    uint8_t k[200], extra, n, i;
    for (; *text; text++) {       /* E before a digit or - is the exponent */
        if (*text == 'E' && (text[1] == '-' || (text[1] >= '0' && text[1] <= '9'))) *o++ = CH_EE;
        else if (*text == '-' && o > t && o[-1] == CH_EE) *o++ = CH_NEG;
        else *o++ = *text;
    }
    *o = 0;
    n = entry_plan(t, SMART_CALC, k, sizeof k, &extra);
    if (n && k[n - 1] == K_ENTER) n--;   /* scripts press ENTER themselves */
    CHECK(n > 0, "plan %s", text);
    for (i = 0; i < n; i++) (i % 3 ? quick : key)(k[i]);
}

/* run the app with the script; the show-work screen polls too */
static void run(void) {
    mock_keys(q, qn);
    mock_polls = 0;
    app_run(app);
    qn = 0;
}

static const char *shown_out(void) { return ascii(app_screen()->out); }

static void scenario(void) {
    static const app_def_t d = {"SIGFIG", "SIGFIGst", SMART_CALC, compute, 0, &mem, sizeof mem};
    int i;
    app = &d;
    mock_state_reset();
    mock_on_poll = on_poll;

    /* 1. 5.890: plain, no grey copy, 4 sig figs */
    type("5.890*1.0");
    quick(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK_STR(shown_out(), "5.9");
    type("5.890");
    quick(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK_STR(shown_out(), "4 sig figs");

    /* 2. edits: arrows, DEL, INS, overwrite, recompute */
    type("12.3*5.00");
    key(K_ENTER);
    key(K_LEFT); key(K_LEFT); key(K_DEL);
    key(K_2ND); key(K_DEL); key(K_7);
    key(K_LEFT); key(K_RIGHT); key(K_2ND); key(K_LEFT); key(K_8); key(K_2ND); key(K_RIGHT);
    key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();

    /* 3. answers with small text, CLEAR after ENTER keeps history */
    type("1500*1.00");
    key(K_ENTER);
    key(K_CLEAR);
    type("4.5E-4");
    key(K_ENTER);
    key(K_MUL); type("2.0");
    key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK(app_screen()->n >= 3, "history kept: %d", app_screen()->n);

    /* 4. long lines wrap; cursor walks across the row break */
    type("1.0+2.0+3.0+4.0+5.0+6.0+7.0+8.0+9.0");
    for (i = 0; i < 12; i++) key(K_LEFT);
    key(K_9);
    for (i = 0; i < 12; i++) key(K_RIGHT);
    key(K_ENTER);
    type("1.0+2.0+3.0+4.0+5.0+6.0+7.");
    key(K_DEL); key(K_DEL);
    key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();

    /* 5. fill the history, scroll back and forth */
    for (i = 0; i < 7; i++) { type("2.0*3.00"); quick(K_ENTER); }
    for (i = 0; i < 12; i++) { key(K_2ND); key(K_UP); }
    for (i = 0; i < 12; i++) { key(K_2ND); key(K_DOWN); }
    key(K_2ND); key(K_UP);
    key(K_5);              /* typing returns to the bottom */
    key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK(max_scroll_seen >= 5, "scrolled back %d rows", max_scroll_seen);
    CHECK(app_screen()->scroll == 0, "back at the bottom");

    /* 6. indicators: 2nd, ALPHA, letter lock, insert */
    key(K_CLEAR);
    key(K_2ND); key(K_2ND);
    key(K_ALPHA); key(K_ALPHA);
    key(K_UP); key(K_UP);
    key(K_2ND); key(K_DEL); key(K_2ND); key(K_DEL);
    key(K_2ND); key(K_MODE);
    run();

    /* 7. CLEAR, CLEAR on empty clears the screen, reopen restores it */
    type("6.02E23*2.0"); key(K_ENTER);
    key(K_CLEAR); key(K_CLEAR);
    type("3.0"); key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK(app_screen()->n == 0, "cleared history: %d", app_screen()->n);
    key(K_ADD); type("1.00"); key(K_ENTER);   /* Ans survived closing */
    key(K_2ND); key(K_MODE);
    run();
    CHECK_STR(ascii(app_entry()->last), "Ans+1.00");
    CHECK_STR(shown_out(), "4.0");

    /* 8. CLEAR on an empty screen quits; the home screen is left clean */
    key(K_CLEAR); key(K_CLEAR); key(K_CLEAR);
    run();
    mock_target(want); plat_home_reset(); mock_target(NULL);
    compare("after quit");
}

static void scenario_work(void) {
    static const app_def_t d = {"SIGFIG", "SIGFIGst", SMART_CALC, compute, 0, &mem, sizeof mem};
    int i;
    app = &d;
    mock_state_reset();
    type("(2.0+3.15)*1.1E2/7.0^2"); key(K_ENTER);
    for (i = 0; i < 3; i++) { type("2.0*3.00"); quick(K_ENTER); }
    key(K_GRAPH);
    key(K_DOWN); key(K_DOWN); key(K_DOWN); key(K_UP); key(K_CLEAR);
    key(K_5); key(K_ENTER);
    key(K_GRAPH);                       /* a lone number has no work: stays home */
    type("5.0*2.0"); key(K_ENTER);
    key(K_GRAPH); key(K_ENTER);
    key(K_2ND); key(K_MODE);
    run();
    CHECK(work_shown >= 2, "show work opened %d", work_shown);
}

static int geometry(int g) {
    static const mock_os_t geo[] = {
        {0, 30, 12, 21, 12, 21, 12},   /* cells touch */
        {1, 32, 12, 20, 11, 17, 11},   /* gaps between cells */
        {4, 31, 12, 21, 12, 19, 14},   /* tall small font */
    };
    mock_os = geo[g];
    scenario();
    scenario_work();
    {
        uint16_t x0, y0, pw, ph, bw, bh;
        ui_grid(&x0, &y0, &pw, &ph, &bw, &bh);
        CHECK(x0 == mock_os.ox && y0 == mock_os.oy && pw == mock_os.pw && ph == mock_os.ph &&
              bw == mock_os.bw && bh == mock_os.bh, "grid %u %u %u %u %u %u", x0, y0, pw, ph, bw, bh);
    }
    CHECK(cursor_seen_on && cursor_seen_off, "cursor blinked");
    CHECK(small_frames > 50, "small grey text drawn in %d frames", small_frames);
    printf("geometry %d: %d frames compared\n", g, frames);
    return t_fail;
}

int main(void) {
    int g, fails = 0;
    /* the grid is measured once per run, so each geometry gets a process */
    for (g = 0; g < 3; g++) {
        pid_t p = fork();
        int st;
        if (p == 0) exit(geometry(g) ? 1 : 0);
        waitpid(p, &st, 0);
        if (!WIFEXITED(st) || WEXITSTATUS(st)) fails++;
    }
    CHECK(fails == 0, "%d geometries failed", fails);
    TEST_END();
}
