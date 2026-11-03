/* font.h - the built-in proportional fonts (PTABLE v2 look) and a fast
 * renderer that draws straight into VRAM. Generated data: font_data.c. */
#ifndef CHEM_FONT_H
#define CHEM_FONT_H
#include <stdint.h>

typedef struct {
    uint8_t h;                 /* line height (ascent + descent) */
    uint8_t first, last;       /* character codes covered */
    const uint8_t *w;          /* advance width per character */
    const uint16_t *off;       /* byte offset of each glyph's bits */
    const uint8_t *bits;       /* rows of w pixels, LSB first, glyph-packed */
} cfont_t;

extern const cfont_t font_small, font_small_bold, font_large, font_large_bold;

#define FONT_LARGE 1
#define FONT_TRANS 2
#define FONT_BOLD  4

uint8_t font_char_width(char c, uint8_t flags);
uint16_t font_width(const char *s, uint8_t flags);
uint8_t font_height(uint8_t flags);
/* draws s at (x, y) top-left, clipped at the right edge; returns its width */
uint16_t font_draw(uint16_t *vram, const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg);
#endif
