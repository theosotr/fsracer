#ifndef TRACE_PROCESSOR_H
#define TRACE_PROCESSOR_H

#include <string>
#include <vector>

#include "AnalyzerExperimental.h"
#include "DependencyInferenceExpAnalyzer.h"
#include "FSAnalyzerExp.h"
#include "FStrace.h"

namespace fstrace {


class TraceProcessor {
// TODO
public:
  TraceProcessor() {
    dep = new analyzer::DependencyInferenceAnalyzerExp(graph::DOT);
    fs = new analyzer::FSAnalyzerExp(analyzer::FSAnalyzerExp::JSON); 
  }
  void ProcessTrace(const TraceNode *trace_node);
  void DumpOut();

private:
  analyzer::AnalyzerExp* dep;
  analyzer::AnalyzerExp* fs;

  void Setup();
  void AnalyzeTrace();
  void DetectFaults();
};


} // namespace fstrace

#endif
