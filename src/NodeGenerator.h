#ifndef NODE_GENERATOR_H
#define NODE_GENERATOR_H

#include <iostream>

#include "TraceGenerator.h"


using namespace trace;

namespace generator_keys {
  const string FUNC_ARGS = "args/";
  const string FUNC_INVOCATIONS = "func_invocations/";
  const string THREADS = "threads/";
  const string OPERATIONS = "operations/";
  const string LAST_CREATED_EVENT = "last_created_event/";
  const string PROMISE_SET = "promises/set";
  const string PROMISE_EVENT = "promises/event";
  const string TIMER_SET = "timers/set";
}


namespace node_utils {
  void AddSubmitOp(void *wrapctx, OUT void **user_data, const string name,
                   size_t async_pos);
}


namespace trace_generator {


class NodeTraceGenerator : public trace_generator::TraceGenerator {
  public:
    string GetName() const {
      return "NodeTrace";
    }

    wrapper_t GetWrappers() const;
    void Start();
    void Stop();
};


} // namespace trace_generator

#endif
