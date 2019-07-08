#ifndef NODE_GENERATOR_H
#define NODE_GENERATOR_H

#include <iostream>

#include "generator.h"

#define WORKER_OFFSET 336


using namespace trace;

namespace generator_keys {
  const string FUNC_ARGS = "args/";
  const string FUNC_INVOCATIONS = "func_invocations/";
  const string THREADS = "threads/";
  const string OPERATIONS = "operations/";
  const string LAST_CREATED_EVENT = "last_created_event/";
  const string PROMISE_SET = "promises/set";
  const string PROMISE_EVENT = "promises/event";
}


namespace node_utils {
  void AddSubmitOp(void *wrapctx, OUT void **user_data, const string name,
                   size_t async_pos);
}


namespace generator {


class NodeTraceGenerator : public generator::Generator {
  public:
    string GetName() {
      return "NodeTrace";
    }

    wrapper_t GetWrappers();
    void Stop();
};


}

#endif
