/* state blobs: round trip and rejection of stale/damaged data */
#include "../src/common/state.h"
#include "../src/common/ui.h"
#include "test.h"

typedef struct { entry_t e; screen_t s; uint8_t page; } app_state_t;

int main(void) {
    static app_state_t a, b;
    static uint8_t blob[sizeof(app_state_t) + STATE_HDR];
    size_t n;
    entry_init(&a.e, SMART_ALL, LOCK_NUM);
    entry_set(&a.e, "25g kcl\x1Cmol");
    screen_clear(&a.s);
    screen_answer(&a.s, "3.35\x1B" "-1 mol", "0.335");
    screen_push(&a.s, a.e.buf);
    a.page = 3;
    n = state_pack(&a, sizeof a, 1, blob);
    CHECK(n == sizeof a + STATE_HDR, "size");
    memset(&b, 0, sizeof b);
    CHECK(state_unpack(blob, n, &b, sizeof b, 1), "unpack");
    CHECK(!memcmp(&a, &b, sizeof a), "round trip");
    CHECK(!state_unpack(blob, n, &b, sizeof b, 2), "other version");
    CHECK(!state_unpack(blob, n - 1, &b, sizeof b, 1), "short");
    blob[STATE_HDR + 5] ^= 0x40;
    CHECK(!state_unpack(blob, n, &b, sizeof b, 1), "damaged");
    CHECK(!state_unpack(NULL, 0, &b, sizeof b, 1), "missing");
    TEST_END();
}
