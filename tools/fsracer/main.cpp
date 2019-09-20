#include <iostream>
#include <optional>

#include "fsracer_cli.h"

#include "Debug.h"
#include "Processor.h"
#include "TraceGeneratorDriver.hpp"



static void
process_args(gengetopt_args_info &args_info, processor::Processor &trace_proc)
{
  if (args_info.dump_trace_given && args_info.output_trace_given) {
    debug::err(CMDLINE_PARSER_PACKAGE)
      << "options '--dump-trace' and '--output-trace' are mutually exclusive";
    exit(EXIT_FAILURE);
  }

  if (args_info.dump_dep_graph_given && args_info.output_dep_graph_given) {
    debug::err(CMDLINE_PARSER_PACKAGE)
      << "options '--dump-dep-graph' and '--output-dep-graph' are mutually"
      << "exclusive";
    exit(EXIT_FAILURE);
  }

  if (args_info.dump_fs_accesses_given && args_info.output_fs_accesses_given) {
    debug::err(CMDLINE_PARSER_PACKAGE)
      << "options '--dump-fs-accesses' and '--output-fs-accesses'"
      << "are mutually exclusive";
  }

  processor::CLIArgs args;
  if (args_info.dump_trace_given) {
    args.dump_trace = true;
  }
  if (args_info.output_trace_given) {
    args.output_trace = args_info.output_trace_arg;
  }
  if (args_info.analyzer_given) {
    for (int i = 0; i < args_info.analyzer_given; i++) {
      args.analyzers.push_back(args_info.analyzer_arg[i]);
    }
  }

  if (args_info.fault_detector_given) {
    args.fault_detector = args_info.fault_detector_arg;
    string fault_detector = args_info.fault_detector_arg;

    if (fault_detector == "race") {
      // The race detector uses two analyzers:
      // (1) the dependency inference analyzer.
      // (2) the analyzer responsible for computing the file accesses
      //     per execution block.
      args.analyzers.push_back("dep-infer");
      args.analyzers.push_back("fs");
    }
  }

  if (args_info.dep_graph_format_given) {
    args.cli_options.AddEntry("dep_graph_format",
                              args_info.dep_graph_format_arg);
  }

  if (args_info.fs_accesses_format_given) {
    args.cli_options.AddEntry("fs_accesses_format",
                              args_info.fs_accesses_format_arg);
  }

  if (args_info.dump_dep_graph_given) {
    args.cli_options.AddEntry("stdout-dep_graph", "true");
  }

  if (args_info.output_dep_graph_given) {
    args.cli_options.AddEntry("output-dep_graph",
                              args_info.output_dep_graph_arg);
  }

  if (args_info.dump_fs_accesses_given) {
    args.cli_options.AddEntry("stdout-fs_accesses", "true");
  }

  if (args_info.output_fs_accesses_given) {
    args.cli_options.AddEntry("output-fs_accesses",
                              args_info.output_fs_accesses_arg);
  }
  trace_proc.SetCLIArgs(args);
}



int
main(int argc, char **argv)
{

  gengetopt_args_info args_info;
  processor::Processor trace_proc;
  // Let's copy the given arguments, so that we do not break things,
  // since we need to pass a const pointer to a function that
  // expects a raw pointer.
  if (cmdline_parser(argc, argv, &args_info) != 0) {
    cmdline_parser_print_help();
    cmdline_parser_free(&args_info);
    exit(EXIT_FAILURE);
  }
  process_args(args_info, trace_proc);
  fstrace::TraceGeneratorDriver trace_gen (args_info.trace_file_arg);
  cmdline_parser_free(&args_info);

  trace_gen.Start();
  if (trace_gen.HasFailed()) {
    debug::err(trace_gen.GetName()) << trace_gen.GetErr();
    exit(EXIT_FAILURE);
  }

  optional<size_t> pid;
  trace_proc.Setup(pid);
  trace_proc.AnalyzeTraces(trace_gen.GetTrace());
  trace_proc.DetectFaults();
  return 0;
}

