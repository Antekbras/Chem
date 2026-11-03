/* plat_mock.c - fake screen, fonts, keyboard and appvars for host tests */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plat_mock.h"
#include "../src/common/state.h"

mock_os_t mock_os = {16, 12};
uint16_t mock_fb[LCD_W * LCD_H];
static uint16_t *tgt = mock_fb;
mock_text_t mock_log[512];
int mock_nlog;

void mock_target(uint16_t *fb) { tgt = fb ? fb : mock_fb; }
uint16_t *plat_vram(void) { return tgt; }

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
            px(x, y, y < STATUS_H ? (uint16_t)0x52AA : 0xFFFF);
}

void plat_home_put(uint8_t row, uint8_t col, const char *s) {
    (void)row; (void)col; (void)s;
}

static uint8_t glyph_w(char ch, uint8_t flags) {
    unsigned c = (unsigned char)ch;
    return (uint8_t)(((flags & PLAT_LARGE) ? 6 + c % 5 : 4 + c % 3) + ((flags & PLAT_BOLD) ? 1 : 0));
}

uint8_t plat_char_width(char c, uint8_t flags) { return glyph_w(c, flags); }

uint16_t plat_text_width(const char *s, uint8_t flags) {
    uint16_t w = 0;
    for (; *s; s++) w = (uint16_t)(w + glyph_w(*s, flags));
    return w;
}

uint8_t plat_text_height(uint8_t flags) { return (flags & PLAT_LARGE) ? mock_os.lh : mock_os.sh; }

uint16_t plat_text(const char *s, uint16_t x, uint8_t y, uint8_t flags, uint16_t fg, uint16_t bg) {
    uint16_t x0 = x;
    uint8_t h = plat_text_height(flags);
    if (x + plat_text_width(s, flags) > LCD_W) {
        printf("mock: text past the right edge: \"%s\" at %u\n", s, x);
        exit(1);
    }
    if (mock_nlog < 512) {
        mock_text_t *t = &mock_log[mock_nlog++];
        strncpy(t->s, s, sizeof t->s - 1);
        t->s[sizeof t->s - 1] = 0;
        t->x = x; t->y = y; t->flags = flags; t->fg = fg; t->bg = bg;
    }
    for (; *s; s++) {
        int i, j, w = glyph_w(*s, flags);
        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++) {
                bool ink = i < w - 1 && j > 0 && j < h - 1 &&
                           hash((unsigned char)*s + (flags & PLAT_LARGE ? 0u : 300u) + (flags & PLAT_BOLD ? 600u : 0u), (unsigned)i, (unsigned)j) % 3 == 0;
                if (ink) px(x + i, y + j, fg);
                else if (!(flags & PLAT_TRANS)) px(x + i, y + j, bg);
            }
        x = (uint16_t)(x + w);
    }
    return (uint16_t)(x - x0);
}

void mock_log_clear(void) { mock_nlog = 0; }

const mock_text_t *mock_find(const char *s) {
    int i;
    for (i = mock_nlog - 1; i >= 0; i--)
        if (!strcmp(mock_log[i].s, s)) return &mock_log[i];
    return NULL;
}

uint8_t plat_battery(void) { return 3; }

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
