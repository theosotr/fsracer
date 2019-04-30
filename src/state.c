#include "dr_api.h"

#include "state.h"


struct Event*
create_event(enum EventType event_type)
{
    struct Event *ev;
    ev = dr_global_alloc(sizeof(struct Event));
    ev->event_value = 0;
    ev->event_type = event_type;
    return ev;
}


void
update_or_create_ev(struct Event *event, enum EventType event_type)
{
    if (event == NULL) {
        event = create_event(event_type);
        return;
    }
    event->event_type = event_type;
}


void
clear_ev(struct Event *event)
{
    if (event != NULL) {
        dr_global_free(event, sizeof(struct Event));
    }
}


struct State *
init_state()
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
