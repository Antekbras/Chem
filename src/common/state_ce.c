/* state_ce.c - appvar storage (calculator only) */
#ifdef __TICE__
#include <string.h>
#include <ti/vars.h>
#include "state.h"

bool state_load(const char *name, void *data, uint16_t size, uint8_t version) {
    int arch = 0;
    var_t *v = os_GetAppVarData(name, &arch);
    if (!v) return false;
    return state_unpack(v->data, v->size, data, size, version);
}

bool state_save(const char *name, const void *data, uint16_t size, uint8_t version) {
    var_t *v;
    os_DelAppVar(name);
    v = os_CreateAppVar(name, (uint16_t)(size + STATE_HDR));
    if (!v) return false;
    state_pack(data, size, version, v->data);
    return true;
}
#endif
