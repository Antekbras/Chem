/* plat_mock.h - host stand-in for the calculator screen and keyboard.
 * The fake OS paints home-screen cells like the real one: a fixed box per
 * cell, black on white, colors ignored, nothing outside the box. */
#ifndef CHEM_PLAT_MOCK_H
#define CHEM_PLAT_MOCK_H
#include <stdint.h>
#include <stdbool.h>
#include "../src/common/plat.h"

typedef struct {
    uint16_t ox, oy;      /* cell (0,0) box origin */
    uint16_t pw, ph;      /* pitch */
    uint16_t bw, bh;      /* box painted per cell */
    uint8_t sh;           /* small font height */
} mock_os_t;

extern mock_os_t mock_os;
extern uint16_t mock_fb[LCD_W * LCD_H];
/* draw into another buffer (for expected frames); NULL = mock_fb */
void mock_target(uint16_t *fb);
/* the OS glyph for ch painted in cell (r,c) of the current target */
void mock_cell(uint8_t r, uint8_t c, char ch);
uint16_t mock_small_width(const char *s);

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
