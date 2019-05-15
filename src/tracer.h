#include <stdlib.h>

#include <iostream>
#include <fstream>


namespace tracer {

/**
 * Class that represents an event.
 *
 * An event is described by a type which is either S, M, W and a positive number.
 * This number actually denotes the priority of the current event relatively to
 * the other events of the same type.
 *
 */
class Event {

  enum EventType {
    S,
    M,
    W
  };

  private:
    enum EventType event_type;
    unsigned int event_value;

  public:
    Event(enum EventType event_type_, unsigned int event_value_):
      event_type(event_type_),
      event_value(event_value_) {  }

    void SetEventValue(unsigned int event_value_) {
      event_value = event_value_;
    }

    void SetEventType(enum EventType event_type_) {
      event_type = event_type_;
    }

    enum EventType GetEventType() {
      return event_type;
    }

    unsigned int GetEventValue() {
      return event_value;
    }
};


/** It creates an event of type S with the specified event value. */
Event *EmitSEvent(unsigned int event_value);

/** It creates an event of type M with the specified event value. */
Event *EmitMEvent(unsigned int event_value);

/** It creates an event of type W with the specified event value. */
Event *EmitWEvent(unsigned int event_value);


/**
 * This class is used to generate traces.
 */
class Tracer {

  private:
    Event *last_event;

    size_t event_count;

    std::ofstream trace_file;

    void SetupTraceFile(std::string trace_file);

  public:
    Tracer(std::string trace_file_name):
    last_event(NULL),
    event_count(0) {
      SetupTraceFile(trace_file_name);
    }

    void EmitEventTrace(unsigned int event_id);

    void EmitLinkTrace(unsigned int source, unsigned int target);

    void EmitBeginTrace(unsigned int event_id);

    void EmitEndTrace();

    void EmitTriggerOpAsync(const char *operation, unsigned int event_id);

    void EmitTriggerOpSync(const char *operation, unsigned int event_id);

    void IncrEventCount() {
      event_count++;
    }

    void ClearLastEvent();

    void SetLastEvent(Event *event) {
      last_event = event;
    }

    void ClearTracer();

};

}
