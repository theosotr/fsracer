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

  public:
    enum EventType {
      S,
      M,
      W
    };

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

    char *ToString(size_t size);

  private:
    enum EventType event_type;
    unsigned int event_value;

};


/** It creates an event of type S. */
Event *EmitSEvent();

/** It creates an event of type M with the specified event value. */
Event *EmitMEvent(unsigned int event_value);

/** It creates an event of type W with the specified event value. */
Event *EmitWEvent(unsigned int event_value);


/**
 * This class is used to generate traces.
 */
class Tracer {

  public:
    Tracer(std::string trace_file_name):
    last_event(NULL),
    event_count(0),
    current_block(0) {
      SetupTraceFile(trace_file_name);
    }

    void EmitNewEventTrace(unsigned int event_id);

    void EmitLinkTrace(unsigned int source, unsigned int target);

    void EmitBeginTrace(unsigned int event_id);

    void EmitEndTrace();

    void EmitTriggerOpAsync(const char *operation, unsigned int event_id);

    void EmitTriggerOpSync(const char *operation, unsigned int event_id);

    void ConstructSEvent() {
      ClearLastEvent();
      last_event = EmitSEvent();
    }

    void ConstructMEvent(unsigned int event_value) {
      ClearLastEvent();
      last_event = tracer::EmitMEvent(event_value);
    }

    void ConstructWEvent(unsigned int event_value) {
      ClearLastEvent();
      last_event = tracer::EmitWEvent(event_value);
    }

    void IncrEventCount() {
      event_count++;
    }

    size_t GetEventCount() {
      return event_count;
    }

    void ClearLastEvent();

    void SetLastEvent(Event *event) {
      last_event = event;
    }

    Event *GetLastEvent() {
      return last_event;
    }

    void ClearTracer();

  private:
    Event *last_event;

    size_t event_count;

    unsigned int current_block;

    std::ofstream trace_file;

    void SetupTraceFile(std::string trace_file);

};

}
