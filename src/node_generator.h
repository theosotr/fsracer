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


namespace node_utils {
  void AddOperation(generator::Generator *trace_gen, Operation *operation,
                    bool is_async);
  void EmitHpath(void *wrapctx, OUT void **user_data, size_t path_pos,
                 size_t clb_pos, enum Hpath::EffectType effect_type,
                 bool follow_symlink);
  void EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
                size_t new_path_pos, size_t clb_pos);
  void EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
                  size_t new_path_pos, size_t clb_pos);
  void EmitSymlink(void *wrapctx, OUT void **user_data, size_t target_path_pos,
                   size_t new_path_pos, size_t clb_pos);
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
