/* ui_compat.h - stand-in for the shared PTABLE-style helpers (ui_text,
 * ui_fill, ui_frame, ...) until src/common/ui.h has them. Compiled out as
 * soon as ui.h defines UI_LARGE; delete this file then. */
#ifndef ATOM_UI_COMPAT_H
#define ATOM_UI_COMPAT_H
#include "ui.h"
#ifndef UI_LARGE
#include <stdint.h>
#define RGB(r, g, b) (uint16_t)((((b) >> 3) << 11) | (((g) >> 2) << 5) | ((r) >> 3))
#define C_WHITE 0xFFFF
#define C_BLACK 0x0000
#define C_SLATE RGB(106, 121, 139)
#define C_NAVY RGB(49, 56, 74)
#define C_BORDER RGB(82, 80, 82)
#define C_SHADOW RGB(123, 121, 123)
#define C_BAND RGB(246, 242, 246)
#define C_SELECT RGB(205, 226, 255)
#define C_GREY RGB(106, 105, 115)
#define UI_LARGE 1
#define UI_TRANS 2
#define UI_BOLD 4
#define ATOM_UI_COMPAT 1
uint16_t ui_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg);
uint16_t ui_text_width(const char *s, uint8_t flags);
void ui_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
void ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
void ui_frame(const char *title, const char *view, const char *const labels[5]);
void ui_page(const char *title, const char *sub, uint16_t color, uint8_t page, uint8_t npages);
uint8_t ui_pick_at(const char *title, const char *const *items, uint8_t n, uint8_t sel);
#endif
#endif
