/* plat.h - the few OS services the UI needs. plat_ce.c implements them on
 * the calculator; tests/plat_mock.c implements them on the host with a
 * fake 320x240 screen so drawing is verified pixel by pixel. */
#ifndef CHEM_PLAT_H
#define CHEM_PLAT_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_W 320
#define LCD_H 240
#define STATUS_H 30            /* OS status bar height */
#define PLAT_TICKS_PER_SEC 32768UL

uint16_t *plat_vram(void);                 /* 320x240 RGB565 */
void plat_home_reset(void);                /* clear screen, draw OS status bar */
/* OS home-screen text (large font, black on white) at (row, col) */
void plat_home_put(uint8_t row, uint8_t col, const char *s);
/* small OS font with colors at pixel (x, y) */
void plat_small(const char *s, uint16_t x, uint8_t y, uint16_t fg, uint16_t bg);
uint16_t plat_small_width(const char *s);
uint8_t plat_small_height(void);
uint8_t plat_getcsc(void);                 /* 0 = no key */
uint32_t plat_ticks(void);

#endif
