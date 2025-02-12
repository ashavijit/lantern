#include <postgres.h>

#include "hnsw/failure_point.h"

#include <inttypes.h> /* PRIu32 */

struct failure_point_state
{
    bool   enabled;
    char   func[ 0x100 ];
    char   name[ 0x100 ];
    uint32 remaining;
};

static struct failure_point_state *failure_point_get_state(void)
{
    static struct failure_point_state state = {};

    return &state;
}

void ldb_failure_point_enable(const char *func, const char *name, uint32 dont_trigger_first_nr)
{
    struct failure_point_state *state = failure_point_get_state();

    if(!LANTERN_FAILURE_POINTS_ARE_ENABLED) {
        elog(WARNING,
             "Can't enable failure point for (func=%s name=%s), "
             "because failure points are disabled in compile time.",
             func,
             name);
    }
    if(state->enabled) {
        elog(WARNING,
             "ldb_failure_point_enable(): another failure point is enabled already."
             " old failure point: func=%s name=%s remaining=%" PRIu32
             " new failure point: func=%s name=%s dont_trigger_first_nr=%" PRIu32,
             state->func,
             state->name,
             state->remaining,
             func,
             name,
             dont_trigger_first_nr);
    }
    if(strlen(func) >= lengthof(state->func)) {
        elog(ERROR,
             "failure point function name is too large: "
             "func=%s strlen(func)=%zu lengthof(state->func)=%zu",
             func,
             strlen(func),
             lengthof(state->func));
    }
    if(strlen(name) >= lengthof(state->name)) {
        elog(ERROR,
             "failure point name is too large: "
             "name=%s strlen(name)=%zu lengthof(state->name)=%zu",
             name,
             strlen(name),
             lengthof(state->name));
    }
    state->enabled = true;
    state->remaining = dont_trigger_first_nr;
    strncpy(state->func, func, lengthof(state->func));
    strncpy(state->name, name, lengthof(state->name));
    elog(INFO, "Failure point (func=%s name=%s) is enabled.", state->func, state->name);
}

bool ldb_failure_point_is_enabled(const char *func, const char *name)
{
    struct failure_point_state *state = failure_point_get_state();

    if(!LANTERN_FAILURE_POINTS_ARE_ENABLED) return false;
    if(!state->enabled) return false;
    if(strcmp(func, state->func) == 0 && strcmp(name, state->name) == 0) {
        if(state->remaining == 0) {
            state->enabled = false;
            elog(INFO, "Failure point (func=%s name=%s) has been triggered.", state->func, state->name);
            return true;
        } else {
            --state->remaining;
        }
    }
    return false;
}

void ldb_failure_point_crash(void)
{
    elog(ERROR, "ldb_failure_point_crash()");
    pg_unreachable();
}
