#include <string.h>
#include "state.h"

static uint8_t sum(const uint8_t *p, uint16_t n) {
    uint8_t s = 0x5A;
    while (n--) s = (uint8_t)((s << 1 | s >> 7) ^ *p++);
    return s;
}

size_t state_pack(const void *data, uint16_t size, uint8_t version, uint8_t *blob) {
    blob[0] = 'C';
    blob[1] = 'S';
    blob[2] = version;
    blob[3] = (uint8_t)size;
    blob[4] = (uint8_t)(size >> 8);
    blob[5] = sum(data, size);
    memcpy(blob + STATE_HDR, data, size);
    return (size_t)size + STATE_HDR;
}

bool state_unpack(const uint8_t *blob, size_t len, void *data, uint16_t size, uint8_t version) {
    if (!blob || len != (size_t)size + STATE_HDR) return false;
    if (blob[0] != 'C' || blob[1] != 'S' || blob[2] != version) return false;
    if ((uint16_t)(blob[3] | blob[4] << 8) != size) return false;
    if (sum(blob + STATE_HDR, size) != blob[5]) return false;
    memcpy(data, blob + STATE_HDR, size);
    return true;
}
