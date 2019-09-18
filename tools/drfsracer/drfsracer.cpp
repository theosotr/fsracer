#include <string.h>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "drfsracer_cli.h"

#include "Analyzer.h"
#include "Debug.h"
#include "DependencyInferenceAnalyzer.h"
#include "Graph.h"
#include "FaultDetector.h"
#include "FSAnalyzer.h"
#include "NodeGenerator.h"
#include "OutWriter.h"
#include "Processor.h"
#include "RaceDetector.h"
#include "DynamoTraceGenerator.h"


static trace_generator::DynamoTraceGenerator *trace_gen;
static processor::Processor *trace_proc;
bool module_loaded = false;


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
  trace_gen->Setup(mod);
  if (!module_loaded) {
    size_t pid = dr_get_thread_id(drcontext);

    size_t buf_size = 100;
    char *cwd_buf = (char *) dr_global_alloc(buf_size * sizeof(char));
    // TODO: Maybe, we should call this function
    // everytime we generate a trace that depends on
    // the current working directory of the process.
    dr_get_current_directory(cwd_buf, buf_size);
    string cwd = cwd_buf; // copy it to a C++ string.
    // Since we have copied the contents of the buffer,
    // let's free memory.
    dr_global_free(cwd_buf, buf_size);
    trace_gen->GetTrace()->SetThreadId(pid);
    trace_gen->GetTrace()->SetCwd(cwd);
    module_loaded = true;
    trace_gen->Start();
    debug::info(trace_gen->GetName())
      << "PID " << pid << ", Start collecting trace...";
  }
}


static void
stop_trace_gen()
{

  if (!trace_gen) {
    return;
  }
  trace_gen->Stop();
  if (!trace_gen->HasFailed()) {
    debug::info(trace_gen->GetName()) << "Trace collected in "
      << trace_gen->GetTraceGenerationTime() << " seconds";
  } else {
    // The trace section has aborted, so we dump the error message.
    debug::err(trace_gen->GetName())
      << "Trace collection aborted: "
      << trace_gen->GetErr();
  }
}


static void
clear_fsracer_setup()
{
  if (trace_proc) {
    delete trace_proc;
  }
  if (trace_gen) {
    delete trace_gen;
  }
}


static void
event_exit(void)
{
  drwrap_exit();
  drmgr_exit();
  drsym_exit();

  // Traces collected. Stop trace generator.
  stop_trace_gen();
  if (trace_gen && !trace_gen->HasFailed()) {
    trace_proc->Setup();
    trace_proc->AnalyzeTraces(trace_gen->GetTrace());
    trace_proc->DetectFaults();
  }
  // Deallocate memory and clear things.
  clear_fsracer_setup();

}


trace_generator::DynamoTraceGenerator *
init_trace_generator(gengetopt_args_info &args_info)
{
  string trace_gen_val = args_info.trace_generator_arg;
  if (trace_gen_val == "node") {
    return new trace_generator::NodeTraceGenerator();
  }
  return nullptr;
}


static void
process_args(gengetopt_args_info &args_info)
{
  if (args_info.dump_trace_given && args_info.output_trace_given) {
    debug::err(CMDLINE_PARSER_PACKAGE)
      << "options '--dump-trace' and '--output-trace' are mutually exclusive";
    dr_exit_process(1);
  }

  if (args_info.dump_dep_graph_given && args_info.output_dep_graph_given) {
    debug::err(CMDLINE_PARSER_PACKAGE)
      << "options '--dump-dep-graph' and '--output-dep-graph' are mutually"
      << "exclusive";
    dr_exit_process(1);
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

  trace_gen = init_trace_generator(args_info);
  trace_proc = new processor::Processor(args);
}


static char **
copy_argv(int argc, const char **argv)
{
  char **c_argv = (char **) malloc(argc * sizeof(argv));
  for (auto i = 0; i < argc; i++) {
    c_argv[i] = strdup(argv[i]);
  }
  return c_argv;
}


void
free_argv(int argc, char **argv)
{
  for (auto i = 0; i < argc; i++) {
    free(argv[i]);
    argv[i] = nullptr;
  }
  free(argv);
  argv = nullptr;
}


DR_EXPORT void
dr_client_main(client_id_t client_id, int argc, const char **argv)
{
  gengetopt_args_info args_info;
  // Let's copy the given arguments, so that we do not break things,
  // since we need to pass a const pointer to a function that
  // expects a raw pointer.
  char **c_argv = copy_argv(argc, argv);
  if (cmdline_parser(argc, c_argv, &args_info) != 0) {
    cmdline_parser_print_help();
    free_argv(argc, c_argv);
    dr_exit_process(1);
  }
  process_args(args_info);
  cmdline_parser_free(&args_info);
  free_argv(argc, c_argv);
  dr_set_client_name("Client for Detecting Data Races in Files", "");
  debug::msg() << "Starting the FSRacer Client...";
  drmgr_init();
  drwrap_init();
  drsym_init(0);
  dr_register_exit_event(event_exit);
  drmgr_register_module_load_event(module_load_event);
}