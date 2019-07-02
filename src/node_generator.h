#ifndef NODE_GENERATOR_H
#define NODE_GENERATOR_H

#include <iostream>

#include "generator.h"


using namespace trace;


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
