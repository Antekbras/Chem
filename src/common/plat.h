/* plat.h - the few OS services the UI needs. plat_ce.c implements them on
 * the calculator; tests/plat_mock.c implements them on the host with a
 * fake 320x240 screen so drawing is verified pixel by pixel. */
#ifndef CHEM_PLAT_H
#define CHEM_PLAT_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_W 320
#define LCD_H 240
#define STATUS_H 30            /* title bar height (replaces the OS status bar) */
#define PLAT_TICKS_PER_SEC 32768UL

/* text flags */
#define PLAT_LARGE 1           /* OS large font (else small) */
#define PLAT_TRANS 2           /* no background */

uint16_t *plat_vram(void);                 /* 320x240, 16 bpp (BGR565 like the OS) */
void plat_home_reset(void);                /* clear the screen the OS way */
/* OS home-screen text (large font, black on white) at (row, col) */
void plat_home_put(uint8_t row, uint8_t col, const char *s);
/* OS font text at pixel (x, y); returns the width drawn */
uint16_t plat_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg);
uint16_t plat_text_width(const char *s, uint8_t flags);
uint8_t plat_text_height(uint8_t flags);
/* shorthands */
#define plat_small(s, x, y, fg, bg) ((void)plat_text((s), (x), (y), 0, (fg), (bg)))
#define plat_large(s, x, y, fg, bg) ((void)plat_text((s), (x), (y), PLAT_LARGE, (fg), (bg)))
#define plat_small_width(s) plat_text_width((s), 0)
#define plat_large_width(s) plat_text_width((s), PLAT_LARGE)
#define plat_small_height() plat_text_height(0)
#define plat_large_height() plat_text_height(PLAT_LARGE)
uint8_t plat_battery(void);                /* 0..4 */
uint8_t plat_getcsc(void);                 /* 0 = no key */
uint32_t plat_ticks(void);

#endif
