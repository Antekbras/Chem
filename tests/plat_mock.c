/* plat_mock.c - fake screen, fonts, keyboard and appvars for host tests */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plat_mock.h"
#include "../src/common/state.h"

mock_os_t mock_os = {0, 30, 12, 21, 12, 21, 12};
uint16_t mock_fb[LCD_W * LCD_H];
static uint16_t *tgt = mock_fb;

void mock_target(uint16_t *fb) { tgt = fb ? fb : mock_fb; }
uint16_t *plat_vram(void) { return mock_fb; }

static unsigned hash(unsigned ch, unsigned x, unsigned y) {
    unsigned h = ch * 2654435761u ^ x * 40503u ^ y * 9973u;
    h ^= h >> 13;
    return h * 2246822519u;
}

static void px(int x, int y, uint16_t c) {
    if (x >= 0 && x < LCD_W && y >= 0 && y < LCD_H) tgt[y * LCD_W + x] = c;
}

void plat_home_reset(void) {
    int x, y;
    for (y = 0; y < LCD_H; y++)
        for (x = 0; x < LCD_W; x++)
            px(x, y, y < STATUS_H ? (uint16_t)(0x52AA + (x / 40 == 3 && y > 10 && y < 20 ? 0x1234 : 0)) : 0xFFFF);
}

void mock_cell(uint8_t r, uint8_t c, char ch) {
    int x0 = mock_os.ox + c * mock_os.pw, y0 = mock_os.oy + r * mock_os.ph, x, y;
    for (y = 0; y < mock_os.bh; y++)
        for (x = 0; x < mock_os.bw; x++) {
            bool ink = ch != ' ' && x > 0 && y > 1 && x < mock_os.bw - 1 && y < mock_os.bh - 2 &&
                       hash((unsigned char)ch, (unsigned)x, (unsigned)y) % 3 == 0;
            px(x0 + x, y0 + y, ink ? 0x0000 : 0xFFFF);
        }
}

void plat_home_put(uint8_t row, uint8_t col, const char *s) {
    if (row >= 10) { printf("mock: home_put row %u\n", row); abort(); }
    for (; *s && col < 26; s++, col++) mock_cell(row, col, *s);
}

static uint8_t small_w(char ch) { return (uint8_t)(5 + (unsigned char)ch % 3); }

uint16_t mock_small_width(const char *s) {
    uint16_t w = 0;
    for (; *s; s++) w = (uint16_t)(w + small_w(*s));
    return w;
}
uint16_t plat_small_width(const char *s) { return mock_small_width(s); }
uint8_t plat_small_height(void) { return mock_os.sh; }

void plat_small(const char *s, uint16_t x, uint8_t y, uint16_t fg, uint16_t bg) {
    for (; *s; s++) {
        int i, j, w = small_w(*s);
        for (j = 0; j < mock_os.sh; j++)
            for (i = 0; i < w; i++)
                px(x + i, y + j, i < w - 1 && j > 0 && j < mock_os.sh - 1 &&
                                 hash((unsigned char)*s + 300u, (unsigned)i, (unsigned)j) % 2 ? fg : bg);
        x = (uint16_t)(x + w);
    }
}

static const uint8_t *keys;
static int nkeys;
void (*mock_on_poll)(void);
int mock_polls, mock_taken;
uint8_t mock_last_key;
uint32_t mock_tick_step = PLAT_TICKS_PER_SEC / 3;
static uint32_t ticks;

void mock_keys(const uint8_t *k, int n) { keys = k; nkeys = n; }

uint8_t plat_getcsc(void) {
    uint8_t k;
    mock_polls++;
    if (mock_on_poll) mock_on_poll();
    if (nkeys <= 0) { printf("mock: ran out of keys\n"); exit(1); }
    k = *keys++;
    nkeys--;
    if (k) { mock_polls = 0; mock_taken++; mock_last_key = k; }
    return k;
}

uint32_t plat_ticks(void) { return ticks += mock_tick_step; }

/* appvars */
static struct { char name[9]; uint8_t blob[4096]; size_t len; } vars[4];

void mock_state_reset(void) { memset(vars, 0, sizeof vars); }

bool state_save(const char *name, const void *data, uint16_t size, uint8_t version) {
    int i;
    for (i = 0; i < 4; i++)
        if (!vars[i].name[0] || !strcmp(vars[i].name, name)) break;
    if (i == 4 || size + STATE_HDR > (int)sizeof vars[i].blob) { printf("mock: appvar too big\n"); exit(1); }
    strncpy(vars[i].name, name, 8);
    vars[i].len = state_pack(data, size, version, vars[i].blob);
    return true;
}

bool state_load(const char *name, void *data, uint16_t size, uint8_t version) {
    int i;
    for (i = 0; i < 4; i++)
        if (vars[i].name[0] && !strcmp(vars[i].name, name))
            return state_unpack(vars[i].blob, vars[i].len, data, size, version);
    return false;
}
