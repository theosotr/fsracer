#ifndef TRACE_PROCESSOR_H
#define TRACE_PROCESSOR_H

#include <optional>
#include <string>
#include <vector>
#include <utility>

#include "Analyzer.h"
#include "FaultDetector.h"
#include "Table.h"
#include "TraceGenerator.h"
#include "OutWriter.h"


namespace processor {

struct CLIArgs {
  std::vector<std::string> analyzers;
  std::optional<std::string> fault_detector;
  table::Table<std::string, std::string> cli_options;
  bool dump_trace;
  std::optional<std::string> output_trace;
  std::optional<std::string> ignore_files_conf;
};


class TraceProcessor {
public:
  TraceProcessor();
  TraceProcessor(CLIArgs cli_args);
  ~TraceProcessor();

  void Setup(std::optional<size_t> pid);

  /// Analyze a single trace node.
  void AnalyzeTrace(const fstrace::TraceNode *trace_node);

  void DetectFaults();

  void DumpAnalysisOutput();

  void SetCLIArgs(CLIArgs cli_args_) {
    cli_args = cli_args_;
  }

private:
  CLIArgs cli_args;
  /// A list of analyzers that operate on traces.
  std::vector<std::pair<analyzer::Analyzer*, writer::OutWriter*>> analyzers;
  /// Component used to detect faults.
  detector::FaultDetector *fault_detector;

  void InitAnalyzers(std::optional<size_t> pid);

  void InitFaultDetector();
};


} // namespace processor


#endif
