#include <assert.h>

#include "Debug.h"
#include "TraceProcessor.h"
#include "Utils.h"


namespace fstrace {


void TraceProcessor::ProcessTrace(const TraceNode *trace_node) {
  if (trace_node) {
    debug::msg() << trace_node->ToString();
  }
}

} // namespace fstrace
