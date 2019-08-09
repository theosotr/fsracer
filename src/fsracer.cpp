#include <string.h>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "cmdline.h"

#include "Analyzer.h"
#include "DependencyInferenceAnalyzer.h"
#include "Graph.h"
#include "FSAnalyzer.h"
#include "NodeGenerator.h"
#include "OutWriter.h"


#define INIT_OUT(arg_prefix)                                               \
  do {                                                                     \
    if (args_info.dump_##arg_prefix##_given) {                             \
      out = new writer::OutWriter(writer::OutWriter::WRITE_STDOUT, "");    \
    }                                                                      \
    if (args_info.output_##arg_prefix##_given) {                           \
      out = new writer::OutWriter(writer::OutWriter::WRITE_FILE,           \
                                  args_info.output_##arg_prefix##_arg);    \
    }                                                                      \
  }                                                                        \
  while (false)                                                            \


using namespace generator;
using namespace analyzer;

/**
 * This struct contains all the information about the current instance
 * of FSracer.
 *
 * Specifically, it contains the generator responsible for creating
 * traces, and a vector of analyzers working on the generated traces.
 */
struct FSracerSetup {
  /// Generator used for creating traces.
  Generator *trace_gen;
  /// A list of analyzers that operate on traces.
  vector<pair<Analyzer*, writer::OutWriter*>> analyzers;

  FSracerSetup(Generator *trace_gen_,
               vector<pair<Analyzer*, writer::OutWriter*>> analyzers_):
    trace_gen(trace_gen_),
    analyzers(analyzers_) { }
};


static FSracerSetup *setup;
bool module_loaded = false;


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
  Generator *trace_gen = setup->trace_gen;
  trace_gen->Start(mod);
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
    cout << trace_gen->GetName() <<
      ": PID " << to_string(pid) <<
      ", Start collecting trace...\n";
  }
}


static void
stop_trace_gen(FSracerSetup *setup)
{

  if (!setup || !setup->trace_gen) {
    return;
  }
  cout << setup->trace_gen->GetName() << ": Trace collected\n";
  setup->trace_gen->Stop();
}


static void
analyze_traces(FSracerSetup *setup)
{
  if (!setup || !setup->trace_gen) {
    return;
  }
  for (auto const &pair_analyzer : setup->analyzers) {
    Analyzer *analyzer = pair_analyzer.first;
    writer::OutWriter *out = pair_analyzer.second;
    // TODO: Currently the analysis of traces is done offline
    // (after the execution of the program).
    //
    // Add support for both offline and online trace analysis.
    cout << analyzer->GetName() << ": Start analyzing traces...\n";
    analyzer->Analyze(setup->trace_gen->GetTrace());
    cout << analyzer->GetName() << ": Analysis is done\n";
    if (out) {
      cout << analyzer->GetName()
        << ": Dumping analysis output to "
        << out->ToString() << "\n";
      analyzer->DumpOutput(out);
    }
  }
}


static void
clear_fsracer_setup(FSracerSetup *setup)
{
  if (!setup) {
    return;
  }
  if (setup->trace_gen) {
    delete setup->trace_gen;
  }
  for (auto &entry : setup->analyzers) {
    Analyzer *analyzer = entry.first;
    if (analyzer) {
      delete analyzer;
    }
  }
  setup->analyzers.clear();
  delete setup;
}


static void
event_exit(void)
{
  drwrap_exit();
  drmgr_exit();
  drsym_exit();

  // Traces collected. Stop trace generator.
  stop_trace_gen(setup);
  // Analyze generated traces.
  analyze_traces(setup);
  // Deallocate memory and clear things.
  clear_fsracer_setup(setup);

}

static graph::GraphFormat
get_graph_format(gengetopt_args_info &args_info)
{
  string graph_format = args_info.dep_graph_format_arg;
  if (graph_format == "dot") {
    return graph::DOT;
  }
  return graph::CSV;
}


static FSAnalyzer::OutFormat
get_fs_out_format(gengetopt_args_info &args_info)
{
  string out_format = args_info.fs_accesses_format_arg;
  if (out_format == "json") {
    return FSAnalyzer::JSON;
  }
  return FSAnalyzer::CSV;
}


static void
init_analyzers(gengetopt_args_info &args_info,
               vector<pair<Analyzer*, writer::OutWriter*>> &analyzers)
{
  Analyzer *analyzer_ptr = nullptr;
  writer::OutWriter *out = nullptr;
  for (int i = 0; i < args_info.analyzer_given; i++) {
    string analyzer = args_info.analyzer_arg[i];
    if (analyzer == "dep-infer") {
      analyzer_ptr = new DependencyInferenceAnalyzer(
          get_graph_format(args_info));
      INIT_OUT(dep_graph);
    }
    if (analyzer == "fs") {
      analyzer_ptr = new FSAnalyzer(get_fs_out_format(args_info));
      INIT_OUT(fs_accesses);
    }
    analyzers.push_back({ analyzer_ptr, out });
    out = nullptr;
  }
}


static void
process_args(gengetopt_args_info &args_info)
{
  if (args_info.dump_trace_given && args_info.output_trace_given) {
    cerr << CMDLINE_PARSER_PACKAGE << ": "
      << "options '--dump-trace' and '--output-trace' are mutually exclusive\n";
    dr_exit_process(1);
  }

  if (args_info.dump_dep_graph_given && args_info.output_dep_graph_given) {
    cerr << CMDLINE_PARSER_PACKAGE << ": "
      << "options '--dump-dep-graph' and '--output-dep-graph' are mutually exclusive\n";
    dr_exit_process(1);
  }

  if (args_info.dump_fs_accesses_given && args_info.output_fs_accesses_given) {
    cerr << CMDLINE_PARSER_PACKAGE << ": "
      << "options '--dump-fs-accesses' and '--output-fs-accesses' are mutually exclusive\n";
  }

  vector<pair<Analyzer*, writer::OutWriter*>> analyzers;
  if (args_info.dump_trace_given) {
    analyzers.push_back({ new DumpAnalyzer(),
        new writer::OutWriter(writer::OutWriter::WRITE_STDOUT, "") });
  }

  if (args_info.output_trace_given) {
    analyzers.push_back({ new DumpAnalyzer(),
        new writer::OutWriter(writer::OutWriter::WRITE_FILE,
                              args_info.output_trace_arg) });
  }
  init_analyzers(args_info, analyzers);
  setup = new FSracerSetup(new NodeTraceGenerator(), analyzers);
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
  dr_printf("Starting the FSRacer Client...\n");
  drmgr_init();
  drwrap_init();
  drsym_init(0);
  dr_register_exit_event(event_exit);
  drmgr_register_module_load_event(module_load_event);
}
