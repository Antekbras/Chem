/* font.c - draw the built-in fonts straight into VRAM (see font.h) */
#include <string.h>
#include "font.h"

#define W 320
#define H 240

static const cfont_t *pick(uint8_t flags) {
    if (flags & FONT_LARGE) return (flags & FONT_BOLD) ? &font_large_bold : &font_large;
    return (flags & FONT_BOLD) ? &font_small_bold : &font_small;
}

uint8_t font_char_width(char ch, uint8_t flags) {
    const cfont_t *f = pick(flags);
    uint8_t c = (uint8_t)ch;
    if (c < f->first || c > f->last || !f->w[c - f->first]) c = '?';
    return f->w[c - f->first];
}

uint16_t font_width(const char *s, uint8_t flags) {
    uint16_t w = 0;
    while (*s) w = (uint16_t)(w + font_char_width(*s++, flags));
    return w;
}

uint8_t font_height(uint8_t flags) { return pick(flags)->h; }

uint16_t font_draw(uint16_t *vram, const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg) {
    const cfont_t *f = pick(flags);
    uint16_t x0 = x;
    uint8_t h = f->h;
    if (y >= H || x >= W) return 0;
    if (y + h > H) h = (uint8_t)(H - y);
    for (; *s; s++) {
        uint8_t c = (uint8_t)*s, gw, r, col, m, used;
        const uint8_t *b;
        uint16_t *row;
        if (c < f->first || c > f->last || !f->w[c - f->first]) c = '?';
        c = (uint8_t)(c - f->first);
        gw = f->w[c];
        if (x + gw > W) break;                 /* clip whole glyphs at the edge */
        b = f->bits + f->off[c];
        row = vram + (uint32_t)y * W + x;
        m = 1;
        used = *b;
        for (r = 0; r < h; r++, row += W) {
            for (col = 0; col < gw; col++) {
                if (used & m) row[col] = fg;
                else if (!(flags & FONT_TRANS)) row[col] = bg;
                m = (uint8_t)(m << 1);
                if (!m) { m = 1; used = *++b; }
            }
        }
        x = (uint16_t)(x + gw);
    }
    return (uint16_t)(x - x0);
}
