/* table.c - periodic table ([Y=]). Arrows move, 1-4 color by family,
 * metal type, block or state, ENTER puts the element on the entry line
 * (config, [GRAPH] = element card), CLEAR goes back. */
#include <string.h>
#include "atom.h"
#include "plat.h"

/* BGR565, like the rest of the UI */
#define RGB(r, g, b) (uint16_t)((((b) >> 3) << 11) | (((g) >> 2) << 5) | ((r) >> 3))
#define WHITE 0xFFFF
#define BLACK 0x0000

#define CW 17
#define CH 17
#define TX 7
#define TY 54
#define FGAP 4

void px_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint16_t *v = plat_vram(), i, j;
    for (j = y; j < y + h && j < LCD_H; j++)
        for (i = x; i < x + w && i < LCD_W; i++) v[(uint32_t)j * LCD_W + i] = color;
}

uint8_t wait_key(void) {
    uint8_t k;
    while (!(k = plat_getcsc())) ;
    return k;
}

static const uint16_t pal_family[10] = {
    RGB(255, 153, 153), RGB(255, 204, 153), RGB(255, 236, 160), RGB(204, 204, 204),
    RGB(170, 221, 204), RGB(170, 238, 136), RGB(153, 230, 255), RGB(204, 170, 255),
    RGB(255, 187, 238), RGB(238, 170, 204)};
static const uint16_t pal_type[4] = {RGB(160, 200, 255), RGB(170, 238, 136), RGB(255, 236, 140), RGB(170, 238, 136)};
static const uint16_t pal_block[4] = {RGB(255, 160, 160), RGB(255, 236, 140), RGB(160, 200, 255), RGB(170, 238, 136)};
static const uint16_t pal_state[4] = {RGB(210, 210, 210), RGB(140, 190, 255), RGB(255, 170, 200), WHITE};
static const char *const view_name[4] = {"1:Family", "2:Type", "3:Block", "4:State"};

static uint8_t block_i(uint8_t z) {
    char b = el_block(z);
    return b == 's' ? 0 : b == 'p' ? 1 : b == 'd' ? 2 : 3;
}

static uint8_t class_of(uint8_t z) {
    switch (ast.view) {
    case 1: return elements[z].cat;
    case 2: return block_i(z);
    case 3: return el_state(z);
    }
    return el_family(z);
}

static const char *class_name(uint8_t z) {
    static char b[8];
    switch (ast.view) {
    case 1: return type_name[elements[z].cat];
    case 2: b[0] = el_block(z); strcpy(b + 1, "-block"); return b;
    case 3: return state_name[el_state(z)];
    }
    return family_name[el_family(z)];
}

static uint16_t color_of(uint8_t z) {
    uint8_t c = class_of(z);
    switch (ast.view) {
    case 1: return pal_type[c];
    case 2: return pal_block[c];
    case 3: return pal_state[c];
    }
    return pal_family[c];
}

/* grid position: rows 0-6 periods, 7-8 the f-block series */
static void pos(uint8_t z, uint8_t *r, uint8_t *c) {
    uint8_t g = el_group(z);
    if (!g) {
        *r = z < 89 ? 7 : 8;
        *c = (uint8_t)(3 + z - (z < 89 ? 57 : 89));
    } else {
        *r = (uint8_t)(el_period(z) - 1);
        *c = (uint8_t)(g - 1);
    }
}

static uint8_t z_at(uint8_t r, uint8_t c) {
    uint8_t z, rr, cc;
    for (z = 1; z <= NUM_ELEMENTS; z++) {
        pos(z, &rr, &cc);
        if (rr == r && cc == c) return z;
    }
    return 0;
}

static void cell(uint8_t z, bool sel) {
    uint8_t r, c;
    uint16_t x, y, bg = sel ? BLACK : color_of(z), w;
    const char *s = elements[z].sym;
    pos(z, &r, &c);
    x = (uint16_t)(TX + c * CW);
    y = (uint16_t)(TY + r * CH + (r >= 7 ? FGAP : 0));
    px_fill(x, y, CW - 1, CH - 1, bg);
    w = plat_small_width(s);
    plat_small(s, (uint16_t)(x + (CW - 1 - w) / 2), (uint8_t)(y + (CH - 1 - plat_small_height()) / 2),
               sel ? WHITE : BLACK, bg);
}

static void info(uint8_t z) {
    char line[UI_COLS + 1], *p;
    p = put_u(line, z);
    *p++ = ' ';
    p = put_s(p, elements[z].sym);
    *p++ = ' ';
    p = put_s(p, elements[z].name);
    while (p < line + UI_COLS) *p++ = ' ';
    *p = 0;
    plat_home_put(0, 0, line);
    px_fill(0, 214, LCD_W, 26, WHITE);
    plat_small(view_name[ast.view], 4, 216, RGB(0, 0, 200), WHITE);
    plat_small(class_name(z), 80, 216, BLACK, WHITE);
    plat_small("1-4 view  ENTER pick  CLEAR back", 4, 228, RGB(110, 110, 110), WHITE);
}

static void draw_all(void) {
    uint8_t z;
    px_fill(0, 52, LCD_W, LCD_H - 52, WHITE);
    for (z = 1; z <= NUM_ELEMENTS; z++) cell(z, z == ast.tz);
    info(ast.tz);
}

/* next element from (r,c) stepping (dr,dc); 0 if none */
static uint8_t step(uint8_t z, int8_t dr, int8_t dc) {
    uint8_t r, c;
    int8_t rr, cc;
    pos(z, &r, &c);
    rr = (int8_t)r; cc = (int8_t)c;
    for (;;) {
        uint8_t n;
        rr = (int8_t)(rr + dr);
        cc = (int8_t)(cc + dc);
        if (rr < 0 || rr > 8 || cc < 0 || cc > 17) return 0;
        n = z_at((uint8_t)rr, (uint8_t)cc);
        if (n) return n;
    }
}

void table_view(void) {
    if (ast.tz < 1 || ast.tz > NUM_ELEMENTS) ast.tz = 1;
    if (ast.view > 3) ast.view = 0;
    plat_home_reset();
    draw_all();
    for (;;) {
        uint8_t k = wait_key(), n = 0;
        switch (k) {
        case K_LEFT: n = step(ast.tz, 0, -1); break;
        case K_RIGHT: n = step(ast.tz, 0, 1); break;
        case K_UP: n = step(ast.tz, -1, 0); break;
        case K_DOWN: n = step(ast.tz, 1, 0); break;
        case K_1: case K_2: case K_3: case K_4:
            ast.view = k == K_1 ? 0 : k == K_2 ? 1 : k == K_3 ? 2 : 3;
            draw_all();
            break;
        case K_ENTER:
            app_enter(elements[ast.tz].sym);
            return;
        case K_CLEAR: case K_MODE: case K_YEQU:
            return;
        }
        if (n) {
            cell(ast.tz, false);
            ast.tz = n;
            cell(n, true);
            info(n);
        }
    }
}
