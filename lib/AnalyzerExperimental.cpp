#include "AnalyzerExperimental.h"


namespace analyzer {

void Analyzer::Analyze(const fstrace::TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}

} // namespace analyzer
