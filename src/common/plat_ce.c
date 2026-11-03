/* plat_ce.c - OS services on the calculator */
#ifdef __TICE__
#include <time.h>
#include <sys/lcd.h>
#include <sys/power.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include "plat.h"
#include "font.h"

uint16_t *plat_vram(void) { return (uint16_t *)lcd_Ram; }

void plat_home_reset(void) {
    os_ClrHomeFull();
}

void plat_home_put(uint8_t row, uint8_t col, const char *s) {
    os_SetCursorPos(row, col);
    os_PutStrLine(s);
}

/* text: the built-in fonts (font.c), drawn straight into VRAM */
uint16_t plat_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg) {
    return font_draw((uint16_t *)lcd_Ram, s, x, y, flags, fg, bg);
}

uint8_t plat_char_width(char c, uint8_t flags) { return font_char_width(c, flags); }
uint16_t plat_text_width(const char *s, uint8_t flags) { return font_width(s, flags); }
uint8_t plat_text_height(uint8_t flags) { return font_height(flags); }

uint8_t plat_battery(void) { return boot_GetBatteryStatus(); }

uint8_t plat_getcsc(void) { return os_GetCSC(); }

uint32_t plat_ticks(void) { return (uint32_t)clock(); }
#endif
