#include <assert.h>

#include "Debug.h"
#include "DependencyInferenceExpAnalyzer.h"
#include "Graph.h"
#include "OutWriter.h"
#include "TraceProcessor.h"
#include "Utils.h"
#include "FSFaultDetector.h"


namespace fstrace {


void TraceProcessor::ProcessTrace(const TraceNode *trace_node) {
  if (!trace_node) {
    return;
  }
  debug::msg() << trace_node->ToString();
  dep->Analyze(trace_node);
  fs->Analyze(trace_node);
}


void TraceProcessor::DumpOut() {
  writer::OutWriter *out = new writer::OutWriter(
      writer::OutWriter::WRITE_FILE, "graph.json");
  analyzer::FSAnalyzerExp *fs_analyzer = dynamic_cast<analyzer::FSAnalyzerExp*>(fs);
  analyzer::DependencyInferenceAnalyzerExp *dep_analyzer = dynamic_cast<analyzer::DependencyInferenceAnalyzerExp*>(dep);
  fs->DumpOutput(out);
  detector::FSFaultDetector detector = detector::FSFaultDetector(
      fs_analyzer->GetOutput(), dep_analyzer->GetOutput());
  detector.Detect();
}


} // namespace fstrace
