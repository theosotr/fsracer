#ifndef NODE_GENERATOR_H
#define NODE_GENERATOR_H

#include <iostream>

#include "generator.h"

#define WORKER_OFFSET 336


using namespace trace;

namespace generator_keys {
  const string FUNC_ARGS = "args/";
  const string THREADS = "threads/";
  const string OPERATIONS = "operations/";
  const string THREAD_OPERATIONS = "thread_operations/";
  const string INCOMPLETE_OPERATIONS = "incomplete_ops/";
  const string LAST_CREATED_EVENT = "last_created_event/";
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
};


}

#endif
