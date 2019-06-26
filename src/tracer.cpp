#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "dr_api.h"

#include "tracer.h"


using namespace std;

namespace tracer {

Event *EmitSEvent() {
  enum Event::EventType event_type = Event::S;
  return new Event(event_type, 0);
}


Event *EmitMEvent(unsigned int event_value) {
  enum Event::EventType event_type = Event::M;
  return new Event(event_type, event_value);
}


Event *EmitWEvent(unsigned int event_value) {
  enum Event::EventType event_type = Event::W;
  return new Event(event_type, event_value);
}

}

using namespace tracer;


char *Event::ToString(size_t size) {
  char *str;
  str = (char *) dr_global_alloc(size);
  switch (event_type) {
      case S:
          sprintf(str, "S %d", event_value);
          return str;
      case M:
          sprintf(str, "M %d", event_value);
          return str;
      case W:
          sprintf(str, "W %d", event_value);
          return str;

  }
  return NULL;
}


void Tracer::SetupTraceFile(string trace_file_name) {
  trace_file.open(trace_file_name, ios::trunc);
}


void Tracer::ClearLastEvent() {
  if (!last_event) {
    return;
  }

  delete last_event;
  last_event = NULL;
}


void Tracer::ClearTracer() {
  ClearLastEvent();
  trace_file.close();
}


void Tracer::EmitNewEventTrace(unsigned int event_id) {
  if (!last_event) {
    return;
  }

  size_t buff_size = 20;
  char *event_str = last_event->ToString(buff_size);
  trace_file << "newEvent " << event_id << " " << event_str << "\n";
  dr_global_free(event_str, buff_size);
}


void Tracer::EmitLinkTrace(unsigned int source, unsigned int target) {
  trace_file << "link " << source << ", " << target << "\n";
}


void Tracer::EmitBeginTrace(unsigned int event_id) {
  trace_file << "begin " << event_id << "\n";
}


void Tracer::EmitEndTrace() {
  trace_file << "end\n";
} 


void Tracer::EmitTriggerOpAsync(const char *operation, unsigned int event_id) {
  // TODO;
}


void Tracer::EmitTriggerOpSync(const char *operation, unsigned int event_id) {
  // TODO
}
