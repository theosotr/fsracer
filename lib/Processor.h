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
};



class Processor {
public:
  Processor();
  Processor(CLIArgs cli_args);
  ~Processor();

  void Setup();

  void AnalyzeTraces(const trace::Trace *trace);

  void DetectFaults();

  void SetCLIArgs(CLIArgs cli_args_) {
    cli_args = cli_args_;
  }

private:
  CLIArgs cli_args;
  /// A list of analyzers that operate on traces.
  std::vector<std::pair<analyzer::Analyzer*, writer::OutWriter*>> analyzers;
  /// Component used to detect faults.
  detector::FaultDetector *fault_detector;

  void InitAnalyzers();

  void InitFaultDetector();
};


} // namespace processor
