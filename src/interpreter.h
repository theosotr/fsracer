#include "state.h"


void start_ev(struct State *state, int event_id);

void end_ev(struct State *state, int event_id);

void creat_ev(struct State *state, enum EventType event_type, int event_id);

void triggerOpSync(struct State *state);

void triggerOpAsync(struct State *state);
