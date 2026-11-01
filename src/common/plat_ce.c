/* plat_ce.c - OS services on the calculator */
#ifdef __TICE__
#include <time.h>
#include <sys/lcd.h>
#include <ti/screen.h>
#include <ti/getcsc.h>
#include "plat.h"

uint16_t *plat_vram(void) { return (uint16_t *)lcd_Ram; }

void plat_home_reset(void) {
    os_ClrHomeFull();
}

void plat_home_put(uint8_t row, uint8_t col, const char *s) {
    os_SetCursorPos(row, col);
    os_PutStrLine(s);
}

void plat_small(const char *s, uint16_t x, uint8_t y, uint16_t fg, uint16_t bg) {
    os_FontSelect(os_SmallFont);
    os_SetDrawFGColor(fg);
    os_SetDrawBGColor(bg);
    os_FontDrawText(s, x, y);
    os_FontSelect(os_LargeFont);
}

uint16_t plat_small_width(const char *s) {
    uint16_t w;
    os_FontSelect(os_SmallFont);
    w = (uint16_t)os_FontGetWidth(s);
    os_FontSelect(os_LargeFont);
    return w;
}

uint8_t plat_small_height(void) {
    uint8_t h;
    os_FontSelect(os_SmallFont);
    h = (uint8_t)os_FontGetHeight();
    os_FontSelect(os_LargeFont);
    return h;
}

uint8_t plat_getcsc(void) { return os_GetCSC(); }

uint32_t plat_ticks(void) { return (uint32_t)clock(); }
#endif
