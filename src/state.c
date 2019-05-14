#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "dr_api.h"

#include "state.h"


struct Event*
create_ev(enum EventType event_type, unsigned int event_value, bool is_promise)
{
    struct Event *ev;
    ev = dr_global_alloc(sizeof(struct Event));
    ev->event_value = event_value;
    ev->event_type = event_type;
    ev->is_promise = is_promise;
    return ev;
}


void
update_or_create_ev(struct Event *event, enum EventType event_type,
                    unsigned int event_value, bool is_promise)
{
    if (event == NULL) {
        event = create_ev(event_type, event_value, is_promise);
        return;
    }
    event->event_type = event_type;
    event->event_value = event_value;
    event->is_promise = is_promise;
}


char *
event_to_str(struct Event *event, size_t len)
{
    if (event == NULL) {
        return NULL;
    }

    char *str;
    str = dr_global_alloc(len);
    switch (event->event_type) {
        case S:
            sprintf(str, "S %d", event->event_value);
            return str;
        case M:
            sprintf(str, "M %d", event->event_value);
            return str;
        case W:
            sprintf(str, "W %d", event->event_value);
            return str;

    }
    return NULL;
}


void
clear_ev(struct Event *event)
{
    if (event != NULL) {
        dr_global_free(event, sizeof(struct Event));
    }
}


void
setup_trace_file(struct State *state, const char *path)
{
    if (state == NULL) {
        return;
    }

    state->tracer = fopen(path, "w");
    if (state->tracer == NULL) {
        dr_fprintf(STDERR,
            "FSRacer Error: Could not open trace file. Errno: %d\n", errno);
    }
}


void
close_trace_file(struct State *state)
{
    if (state == NULL) {
        return;
    }

    if (state->tracer == NULL) {
        return;
    }
    if (fclose(state->tracer)) {
        dr_fprintf(STDERR,
            "FSRacer Error: Cannot close trace file. Errno %d\n", errno);
    }
}


/**
 * Just a wrapper to vfprintf that checks that the state->tracer points
 * to a file stream.
 */
int
write_trace(struct State *state, const char *fmt, ...)
{
    if (state == NULL) {
        return -1;
    }

    if (state->tracer == NULL) {
        dr_fprintf(STDERR,
            "FSRacer Error: Cannot write to trace file. Errno: %d\n", errno);
        return -1;
    }
    va_list ap;
    ssize_t res;
    va_start(ap, fmt);
    res = vfprintf(state->tracer, fmt, ap);
    va_end(ap);
    return res;
}


struct State *
init_state(void)
{
    struct State *state;
    state = dr_global_alloc(sizeof(struct State));
    state->current_ev = 1;
    state->last_ev_created = NULL;
    return state;
}


void
clear_state(struct State *state)
{
    if (state == NULL) {
        return;
    }
    clear_ev(state->last_ev_created);
    dr_global_free(state, sizeof(struct State));
}


struct Event *
last_event(struct State *state)
{
    return state->last_ev_created;
}


void
set_current_ev(struct State *state, int current_ev)
{
    if (state != NULL) {
        state->current_ev = current_ev;
    }
}


void
set_last_ev(struct State *state, struct Event *event)
{
    state->last_ev_created = event;
}


void
reset_event(struct State *state)
{
    if (state != NULL) {
        clear_ev(state->last_ev_created);
        state->last_ev_created = NULL;
        state->current_ev = 0;
    }
}
