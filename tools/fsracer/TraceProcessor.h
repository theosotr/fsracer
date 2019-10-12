#ifndef TRACE_PROCESSOR_H
#define TRACE_PROCESSOR_H

#include <string>

#include "FStrace.h"

namespace fstrace {


class TraceProcessor {
// TODO
public:
  void ProcessTrace(const TraceNode *trace_node);

private:
  void AnalyzeTrace();
  void DetectFaults();
};


} // namespace fstrace

#endif
