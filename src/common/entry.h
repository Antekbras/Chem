/* entry.h - one-line editor with letter-lock / number-lock.
 *
 *   ▲ or ▼      toggle letter-lock <-> number-lock
 *   ◄ ►         move cursor (2nd: start / end)
 *   DEL, 2nd+DEL (INS), CLEAR   like the home screen
 *   [STO→]      always types →          ALPHA+key  one OS letter (X = ALPHA+STO→)
 *   2nd+[,]     E exponent              2nd+[(-)]  Ans      2nd+ENTER  last entry
 *   [,]         space in either lock;   [0] is space in letter-lock
 *   3 9 + .     same in either lock
 *   smart lock: in number-lock MATH APPS PRGM x⁻¹ SIN COS TAN ^ x² LOG LN
 *               type A B C D E F G H I N S (per-app mask)
 *   ENTER computes; the line stays editable. After ENTER a typed character
 *   starts a new line; an operator (+ - × ÷ ^ ² →) starts it with "Ans".
 * Letters are stored lowercase; apps auto-case when they parse. */
#ifndef CHEM_ENTRY_H
#define CHEM_ENTRY_H

#include <stdint.h>
#include <stdbool.h>
#include "keys.h"

#define ENTRY_MAX 96

enum { LOCK_NUM = 0, LOCK_LET = 1 };

/* smart-lock masks: bit (letter - 'a') */
#define SMART_BIT(c) (1UL << ((c) - 'a'))
#define SMART_ALL  (SMART_BIT('a') | SMART_BIT('b') | SMART_BIT('c') | SMART_BIT('d') | \
                    SMART_BIT('e') | SMART_BIT('f') | SMART_BIT('g') | SMART_BIT('h') | \
                    SMART_BIT('i') | SMART_BIT('n') | SMART_BIT('s'))
/* calculator apps keep ^ and x² as math keys */
#define SMART_CALC (SMART_ALL & ~(SMART_BIT('h') | SMART_BIT('i')))
#define SMART_NONE 0UL

/* entry_key() results */
enum {
    EK_NONE = 0,     /* nothing changed */
    EK_CHANGED,      /* text/cursor/lock changed: redraw line */
    EK_NEWLINE,      /* key started a new line after ENTER: push old line to history first */
    EK_ENTER,        /* compute */
    EK_GRAPH,        /* show work / field 5 */
    EK_FKEY,         /* [Y=] [WINDOW] [ZOOM] [TRACE]: field in e->fkey (0..3) */
    EK_CLEAR_EMPTY,  /* CLEAR on an empty line: clear the screen */
    EK_SCROLL_UP,    /* 2nd+▲ : scroll history */
    EK_SCROLL_DOWN,
    EK_QUIT,         /* 2nd+MODE */
    EK_OTHER         /* unhandled key in e->key (MODE, VARS, STAT, ...) */
};

typedef struct {
    char buf[ENTRY_MAX + 1];
    char last[ENTRY_MAX + 1];  /* previous entry (2nd+ENTER) */
    uint8_t len, cur;
    uint8_t lock, lock_default;
    bool second, alpha, ins;
    bool done;                 /* ENTER pressed and answer shown */
    bool ans_ops;              /* operator on an empty/done line inserts Ans */
    uint32_t smart;            /* smart-lock letter mask */
    uint8_t key, fkey;         /* last key, function-key index */
} entry_t;

void entry_init(entry_t *e, uint32_t smart, uint8_t lock_default);
void entry_clear(entry_t *e);
void entry_set(entry_t *e, const char *text);
/* call after ENTER has been handled: marks the line done, resets the lock */
void entry_done(entry_t *e);
uint8_t entry_key(entry_t *e, uint8_t key);
/* insert text at the cursor (a picked list item); after ENTER it starts a
 * new line like a typed key. Returns EK_CHANGED or EK_NEWLINE. */
uint8_t entry_insert(entry_t *e, const char *text);

/* character a key types (0 = none) */
char entry_char(uint8_t key, uint8_t lock, bool alpha, bool second, uint32_t smart);

/* ---- key-count planner (tests and the final key-count pass) ----
 * Plans the shortest key sequence that types `text` into an empty line
 * starting in number-lock. Lines separated by " ; " are entered one after
 * another, each ended by ENTER. Returns the number of keys, writes them to
 * keys[] (if not NULL). *extra receives the keys that are not data
 * (ENTER, →, lock toggles, 2nd, ALPHA). Returns 0 if not typeable. */
uint8_t entry_plan(const char *text, uint32_t smart, uint8_t *keys, uint8_t max, uint8_t *extra);

#endif
