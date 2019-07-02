#ifndef NODE_GENERATOR_H
#define NODE_GENERATOR_H

#include <iostream>

#include "generator.h"


using namespace trace;

namespace generator_keys {
  const string FUNC_ARGS = "args/";
  const string INCOMPLETE_OPERATIONS = "incomplete_ops/";
  const string LAST_CREATED_EVENT = "last_created_event/";
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
