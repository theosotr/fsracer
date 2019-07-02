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
typedef map<string, pair<pre_clb_t, post_clb_t>> wrapper_t;



namespace generator {


class Generator {
  public:
    Generator() {
      trace = new Trace();
      current_block = NULL;
      event_pending = false;
      event_count = 0;
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

    map<string, void*> GetStore() {
      return store;
    }

    size_t GetEventCount() {
      return event_count;
    }

    size_t GetLastEventId() {
      return last_event_id;
    }

    void IncrEventCount() {
      event_count++;
    }

    void SetTrace(Trace *trace_) {
      trace = trace_;
    }

    void SetCurrentBlock(Block *block) {
      current_block = block;
    }

    void SetLastEventId(size_t event_id) {
      last_event_id = event_id;
    }

    bool IsEventPending() {
      return event_pending;
    }

    void NewLastEvent(Event::EventType event_type,
                      unsigned int event_value) {
      last_event = Event(event_type, event_value);
      event_pending = true;
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


    void AddToStore(string key, void *value);
    void *GetStoreValue(string key);
    virtual void Start(const module_data_t *mod);
    virtual wrapper_t GetWrappers();
    virtual string GetName();


  private:
    Trace *trace;
    Block *current_block;
    bool event_pending;
    Event last_event;
    size_t event_count;
    map<string, void*> store;
    size_t last_event_id;
    map<string, Operation*> incomplete_ops;

    void RegisterFunc(const module_data_t *mod, string func_name,
                      pre_clb_t pre, post_clb_t post);
};

}


namespace generator_utils {

generator::Generator *GetTraceGenerator(void **data);

}

#endif
