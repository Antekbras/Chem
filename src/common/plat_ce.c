/* plat_ce.c - OS services on the calculator */
#ifdef __TICE__
#include <time.h>
#include <sys/lcd.h>
#include <sys/power.h>
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

static void font(uint8_t flags) {
    os_FontSelect((flags & PLAT_LARGE) ? os_LargeFont : os_SmallFont);
}

uint16_t plat_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg) {
    uint16_t end;
    font(flags);
    os_SetDrawFGColor(fg);
    os_SetDrawBGColor(bg);
    end = (uint16_t)((flags & PLAT_TRANS) ? os_FontDrawTransText(s, x, y) : os_FontDrawText(s, x, y));
    os_FontSelect(os_LargeFont);
    return (uint16_t)(end - x);
}

uint16_t plat_text_width(const char *s, uint8_t flags) {
    uint16_t w;
    font(flags);
    w = (uint16_t)os_FontGetWidth(s);
    os_FontSelect(os_LargeFont);
    return w;
}

uint8_t plat_text_height(uint8_t flags) {
    uint8_t h;
    font(flags);
    h = (uint8_t)os_FontGetHeight();
    os_FontSelect(os_LargeFont);
    return h;
}

uint8_t plat_battery(void) { return boot_GetBatteryStatus(); }

uint8_t plat_getcsc(void) { return os_GetCSC(); }

uint32_t plat_ticks(void) { return (uint32_t)clock(); }
#endif
