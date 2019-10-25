#include <optional>

#include "Debug.h"
#include "Graph.h"
#include "DependencyInferenceAnalyzer.h"
#include "FSAnalyzer.h"
#include "FSFaultDetector.h"
#include "TraceProcessor.h"


#define INIT_OUT(arg_prefix)                                               \
  do {                                                                     \
    std::string key = #arg_prefix;                                         \
    if (cli_args.cli_options.GetValue("stdout-" + key).has_value()) {      \
      out = new writer::OutWriter(writer::OutWriter::WRITE_STDOUT, "");    \
    }                                                                      \
    std::optional<std::string> val = cli_args.cli_options.GetValue(        \
        "output-" + key);                                                  \
    if (val.has_value()) {                                                 \
      std::string filename = val.value();                                  \
      if (pid.has_value()) {                                               \
        filename += std::to_string(pid.value());                           \
      }                                                                    \
      out = new writer::OutWriter(writer::OutWriter::WRITE_FILE,           \
                                  filename);                               \
    }                                                                      \
  }                                                                        \
  while (false)                                                            \


namespace processor {
  

static graph::GraphFormat
get_graph_format(const CLIArgs &cli_args)
{
  std::optional<std::string> val = cli_args.cli_options.GetValue(
      "dep_graph_format");
  if (!val.has_value()) {
    return graph::DOT;
  }
  if (val.value() == "dot") {
    return graph::DOT;
  }
  return graph::CSV;
}


static analyzer::FSAnalyzer::OutFormat
get_fs_out_format(const CLIArgs &cli_args)
{
  std::optional<std::string> val = cli_args.cli_options.GetValue(
      "fs_accesses_format");
  if (!val.has_value()) {
    return analyzer::FSAnalyzer::JSON;
  }
  if (val.value() == "json") {
    return analyzer::FSAnalyzer::JSON;
  }
  return analyzer::FSAnalyzer::CSV;
}


TraceProcessor::TraceProcessor():
  fault_detector(nullptr) {  }


TraceProcessor::TraceProcessor(CLIArgs args):
  cli_args(args) {  }


TraceProcessor::~TraceProcessor() {
  for (auto &entry : analyzers) {
    analyzer::Analyzer *analyzer = entry.first;
    if (analyzer) {
      delete analyzer;
    }      
  }
  analyzers.clear();
  if (fault_detector) {
    delete fault_detector;        
  }
}


void TraceProcessor::Setup(std::optional<size_t> pid) {
  InitAnalyzers(pid);
}



void TraceProcessor::InitAnalyzers(std::optional<size_t> pid) {

  analyzer::Analyzer *analyzer_ptr = nullptr;
  writer::OutWriter *out = nullptr;
  if (cli_args.dump_trace || cli_args.output_trace.has_value()) {
    analyzer_ptr = new analyzer::DumpAnalyzer();
    if (cli_args.dump_trace) {
      out = new writer::OutWriter(writer::OutWriter::WRITE_STDOUT, "");
    }
    if (cli_args.output_trace.has_value()) {
      std::string filename = cli_args.output_trace.value();
      if (pid.has_value()) {
        filename += std::to_string(pid.value());
      }
      out = new writer::OutWriter(writer::OutWriter::WRITE_FILE,
                                  filename);
    }
    analyzers.push_back({ analyzer_ptr, out });
    analyzer_ptr = nullptr;
    out = nullptr;
  }
  for (auto const &analyzer_str : cli_args.analyzers) {
    if (analyzer_str == "dep-infer") {
      analyzer_ptr = new analyzer::DependencyInferenceAnalyzer(
          get_graph_format(cli_args));
      INIT_OUT(dep_graph);
    }
    if (analyzer_str == "fs") {
      analyzer_ptr = new analyzer::FSAnalyzer(
          get_fs_out_format(cli_args));
      INIT_OUT(fs_accesses);
    }
    analyzers.push_back({ analyzer_ptr, out });
    out = nullptr;
  }
}


void TraceProcessor::InitFaultDetector() {
  int offset = 0;
  if (cli_args.dump_trace || cli_args.output_trace.has_value()) {
    offset = 1;
  }
  if (cli_args.fault_detector.has_value()) {
    std::string fault_detector_str = cli_args.fault_detector.value();
    if (fault_detector_str == "fs") {
      // We know the first analyzer corresponds to the `dep-infer` analyzer,
      // while the second one is the `fs` analyzer.
      analyzer::DependencyInferenceAnalyzer *dep_analyzer =
        static_cast<analyzer::DependencyInferenceAnalyzer*>(
            analyzers[offset + 0].first);
      analyzer::FSAnalyzer *fs_analyzer =
        static_cast<analyzer::FSAnalyzer*>(analyzers[offset + 1].first);
      fault_detector = new detector::FSFaultDetector(
          fs_analyzer->GetFSAccesses(),
          dep_analyzer->GetDependencyGraph(),
          fs_analyzer->GetWorkingDir(),
          fs_analyzer->GetDirectories());
    }
  }
}


void TraceProcessor::AnalyzeTrace(const fstrace::TraceNode *trace) {
  for (auto const &pair_analyzer : analyzers) {
    analyzer::Analyzer *analyzer_ptr = pair_analyzer.first;
    analyzer_ptr->Analyze(trace);
  }
}


void TraceProcessor::DumpAnalysisOutput() {
  for (auto const &pair_analyzer : analyzers) {
    analyzer::Analyzer *analyzer_ptr = pair_analyzer.first;
    writer::OutWriter *out = pair_analyzer.second;
    if (!analyzer_ptr) {
      continue;
    }
    if (out) {
      debug::info(analyzer_ptr->GetName())
        << "Dumping analysis output to "
        << out->ToString();
      analyzer_ptr->DumpOutput(out);
    }
  }
}


void TraceProcessor::DetectFaults() {
  InitFaultDetector();
  if (!fault_detector) {
    return;
  }
  debug::info(fault_detector->GetName())
    << "Detecting faults...";
  fault_detector->Detect();
  debug::info(fault_detector->GetName()) << "Faults detected in "
    << fault_detector->GetAnalysisTime() << "ms";
}


} // namespace processor

