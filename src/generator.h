#ifndef GENERATOR_H
#define GENERATOR_H

#include <iostream>
#include <map>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "trace.h"



using namespace trace;
using namespace std;


typedef void (*pre_clb_t)(void *wrapctx, OUT void **user_data);
typedef void (*post_clb_t)(void *wrapctx, void *user_data);


namespace generator {


class Generator {
  public:
    Generator() {
      trace = new Trace();
      current_block = NULL;
      event_pending = false;
    }

    ~Generator() {
      if (trace) {
        delete trace; 
      }
    }

    Trace *GetTrace() {
      return trace;
    }

    Block *GetCurrentBlock() {
      return current_block;
    } 

    Event GetLastEvent() {
      return last_event;
    }

    void SetTrace(Trace *trace_) {
      trace = trace_;
    }

    void SetCurrentBlock(Block *block) {
      current_block = block;
    }

    bool IsEventPending() {
      return event_pending;
    }

    void NewLastEvent(Event::EventType event_type,
                      unsigned int event_value) {
      last_event = Event(event_type, event_value);
      event_pending = true;
    }

    void RegisterFunc(const module_data_t *mod, string func_name, pre_clb_t pre,
                      post_clb_t post);

    virtual void Start(const module_data_t *mod);


  private:
    Trace *trace;
    Block *current_block;
    bool event_pending;
    Event last_event;
};


class NodeTraceGenerator : public Generator {
  public:
    NodeTraceGenerator():
      Generator() {
        event_count = 0;
      }

    void IncrEventCount() {
      event_count++;
    }

    size_t GetEventCount() {
      return event_count;
    }

    void AddIncompleteOp(string key, Operation *op) {
      incomplete_ops[key] = op;
    }

    Operation *GetIncompleteOp(string key) {
      map<string, Operation*>::iterator it;
      it = incomplete_ops.find(key);
      if (it == incomplete_ops.end()) {
        return nullptr;
      } else {
        return it->second;
      }
    }

    void SetLastEventId(unsigned int last_event_id_) {
      last_event_id = last_event_id_;
    }

    unsigned int GetLastEventId() {
      return last_event_id;
    }

  private:
    size_t event_count;

    unsigned int last_event_id;

    map<string, Operation*> incomplete_ops; 

};


}


#endif
