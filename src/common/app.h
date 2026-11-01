/* app.h - shared app shell: state restore, home-screen loop, show work.
 * An app supplies a compute callback; everything else is common. */
#ifndef CHEM_APP_H
#define CHEM_APP_H

#include "ui.h"
#include "work.h"

typedef struct {
    const char *title;      /* show-work header, e.g. "SIGFIG" */
    const char *var;        /* appvar for saved state, e.g. "SIGFIGst" */
    uint32_t smart;         /* SMART_ALL / SMART_CALC */
    /* fill out (big, right aligned) and small (plain decimal); add work lines */
    void (*compute)(const char *in, char *out, char *small, work_t *w);
    /* optional: [Y=]..[TRACE] pressed (field 0..3) */
    void (*field)(uint8_t n);
    /* optional app data kept across runs (e.g. SIGFIG's Ans), <= APP_DATA_MAX */
    void *data;
    uint16_t data_size;
} app_def_t;

#define APP_DATA_MAX 48

/* insert text at the cursor from a field callback (picked list item) */
void app_insert(const char *text);

/* optional, before app_run: labels for a [Y=]..[GRAPH] bar kept on the
 * bottom row (5 labels of <= 5 chars); the line history gets 9 rows */
void app_labels(const char *const labels[5]);

/* run until 2nd+QUIT (or CLEAR on an empty screen); saves state on exit */
void app_run(const app_def_t *d);
/* the same loop in steps (host tests drive these) */
void app_open(const app_def_t *d);
bool app_key(const app_def_t *d, uint8_t key);   /* false = quit */
void app_close(const app_def_t *d);
const entry_t *app_entry(void);
const screen_t *app_screen(void);
const work_t *app_work(void);
/* from a field callback: put `text` on the entry line and compute it, as if
 * typed + ENTER (the finished line before it goes to the history) */
void app_enter(const char *text);

/* Echo a typed line with auto-case, for the stubs and for show-work:
 * "250.0ml .640m nacl>g" -> "250.0mL .640M NaCl→g". Numbers keep their
 * digits, units and formulas are canonicalised, `prefer` = FP_COMPOUND or
 * FP_ELEMENT. Returns length. */
uint8_t app_echo(const char *in, uint8_t prefer, char *out, uint8_t max);

#endif
