/* state.h - apps reopen to their last screen: each app keeps one appvar
 * (e.g. "SIGFIGst") holding a versioned blob of its own state struct. */
#ifndef CHEM_STATE_H
#define CHEM_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define STATE_HDR 6   /* magic 'C','S', version, size lo, size hi, checksum */

/* pure part (host-tested): blob = header + data */
size_t state_pack(const void *data, uint16_t size, uint8_t version, uint8_t *blob);
/* false if the blob is missing, damaged, or from another version/size */
bool state_unpack(const uint8_t *blob, size_t blob_len, void *data, uint16_t size, uint8_t version);

/* calculator part (state_ce.c): appvar named `name` (<= 8 chars) */
bool state_load(const char *name, void *data, uint16_t size, uint8_t version);
bool state_save(const char *name, const void *data, uint16_t size, uint8_t version);

#endif
