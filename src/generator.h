#ifndef GENERATOR_H
#define GENERATOR_H


#include <iostream>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "trace.h"



using namespace trace;


typedef void (*pre_clb_t)(void *wrapctx, OUT void **user_data);
typedef void (*post_clb_t)(void *wrapctx, void *user_data);


namespace generator {


class Generator {
  public:
    Generator() {
      trace = new Trace();
      current_block = NULL;
      last_event = NULL;
    }

    Trace *GetTrace() {
      return trace;
    }

    Block *GetCurrentBlock() {
      return current_block;
    } 

    Event *GetLastEvent() {
      return last_event;
    }

    void SetTrace(Trace *trace_) {
      trace = trace_;
    }

    void SetCurrentBlock(Block *block) {
      current_block = block;
    }

    void SetLastEvent(Event *event) {
      last_event = event;
    }

    void RegisterFunc(const module_data_t *mod, string func_name, pre_clb_t pre,
                      post_clb_t post);

    virtual void Start(const module_data_t *mod);


  private:
    Trace *trace;
    Block *current_block;
    Event *last_event;
};


class NodeTraceGenerator : public Generator {
  public:
    void IncrEventCount() {
      event_count++;
    }

    size_t GetEventCount() {
      return event_count;
    }

  private:
    size_t event_count;

};


}


#endif
