#include <regex>

#include "TraceProcessor.h"
#include "Utils.h"

#include "Debug.h"


namespace fstrace {


void TraceProcessor::ProcessTrace(const TraceNode *trace_node) {
  if (trace_node) {
    debug::msg("TraceNode") << trace_node->ToString();
  }
}


} // namespace fstrace
