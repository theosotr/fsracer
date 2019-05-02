enum EventType {
  S,
  M,
  W
};


struct Event {
  enum EventType event_type;
  unsigned int event_value;
};


struct State {
  int current_ev;
  struct Event *last_ev_created;
};


struct Event *create_ev(enum EventType event_type, unsigned int event_value);

void update_or_create_ev(struct Event *event, enum EventType event_type,
                         unsigned int event_value);

char *event_to_str(struct Event *event, size_t len);

void clear_ev(struct Event *event);

struct State *init_state(void);

void clear_state(struct State *state);

struct Event *last_event(struct State *state);

void set_current_ev(struct State *state, int current_event);

void set_last_ev(struct State *state, struct Event *event);

void reset_event(struct State *state);