/* plat_mock.h - host stand-in for the calculator screen, fonts, keyboard
 * and appvars. Fonts are fake but deterministic (each glyph has its own
 * pixel pattern), so a wrong or stale character shows up as a pixel diff. */
#ifndef CHEM_PLAT_MOCK_H
#define CHEM_PLAT_MOCK_H
#include <stdint.h>
#include <stdbool.h>
#include "../src/common/plat.h"

typedef struct {
    uint8_t lh, sh;       /* large / small font heights */
} mock_os_t;

extern mock_os_t mock_os;
extern uint16_t mock_fb[LCD_W * LCD_H];
/* draw into another buffer (for expected frames); NULL = mock_fb */
void mock_target(uint16_t *fb);

/* every text draw since mock_log_clear(), for "is X on screen" checks */
typedef struct { char s[120]; uint16_t x; uint8_t y, flags; uint16_t fg, bg; } mock_text_t;
extern mock_text_t mock_log[512];
extern int mock_nlog;
void mock_log_clear(void);
/* the last draw of exactly s, or NULL */
const mock_text_t *mock_find(const char *s);

/* keys: the queue is read one entry per poll, 0 = no key this poll.
 * on_poll runs at every poll before the key is taken. */
void mock_keys(const uint8_t *k, int n);
extern void (*mock_on_poll)(void);
extern int mock_polls;          /* polls since the last key was returned */
extern int mock_taken;          /* keys returned so far */
extern uint8_t mock_last_key;
extern uint32_t mock_tick_step; /* ticks added per plat_ticks() call */

/* appvar store */
void mock_state_reset(void);
#endif
