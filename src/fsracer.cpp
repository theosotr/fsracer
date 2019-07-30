#include <string.h>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "cmdline.h"

#include "Analyzer.h"
#include "DependencyInferenceAnalyzer.h"
#include "NodeGenerator.h"
#include "OutWriter.h"


using namespace generator;
using namespace analyzer;


struct FSracerSetup {
  /// Generator used for creating traces.
  Generator *trace_gen;
  /// A list of analyzers that operate on traces.
  vector<Analyzer*> analyzers;

  FSracerSetup(Generator *trace_gen_, vector<Analyzer*> analyzers_):
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
  size_t pid = dr_get_thread_id(drcontext);
  trace_gen->GetTrace()->SetThreadId(pid);

  if (!module_loaded) {
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
  for (auto const &analyzer : setup->analyzers) {
    // TODO: Currently the analysis of traces is done offline
    // (after the execution of the program).
    //
    // Add support for both offline and online trace analysis.
    cout << analyzer->GetName() << ": Start analyzing traces...\n";
    analyzer->Analyze(setup->trace_gen->GetTrace());
    cout << analyzer->GetName() << ": Analysis is done\n";
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
  for (auto i = 0; i < setup->analyzers.size(); i++) {
    if (setup->analyzers[i]) {
      delete setup->analyzers[i];
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

  stop_trace_gen(setup);
  analyze_traces(setup);
  clear_fsracer_setup(setup);

}

static DependencyInferenceAnalyzer::GraphFormat
get_graph_format(gengetopt_args_info &args_info)
{
  string graph_format = args_info.dep_graph_format_arg;
  if (graph_format == "dot") {
    return DependencyInferenceAnalyzer::DOT;
  }
  return DependencyInferenceAnalyzer::CSV;
}


static void
process_args(gengetopt_args_info &args_info)
{
  if (args_info.dump_trace_given && args_info.output_trace_given) {
    // TODO add an erroneous message to the user.
    cerr << CMDLINE_PARSER_PACKAGE << ": "
      << "options '-dump-trace' and '-output-trace' are mutually exclusive\n";
    exit(-1);
  }

  if (args_info.dump_dep_graph_given && args_info.output_dep_graph_given) {
    // TODO add an erroneous message to the user.
    cerr << CMDLINE_PARSER_PACKAGE << ": "
      << "options '-dump-dep-graph' and '-output-dep-graph' are mutually exclusive\n";
    exit(-1);
  }

  vector<Analyzer*> analyzers;
  if (args_info.dump_trace_given) {
    analyzers.push_back(new DumpAnalyzer(writer::OutWriter::WRITE_STDOUT, ""));
  }

  if (args_info.output_trace_given) {
    analyzers.push_back(new DumpAnalyzer(
        writer::OutWriter::WRITE_FILE, args_info.output_trace_orig));
  }
  if (args_info.analyzer_given) {
    string analyzer = args_info.analyzer_orig;
    if (analyzer == "dep-infer") {
      if (args_info.dump_dep_graph_given) {
        analyzers.push_back(new DependencyInferenceAnalyzer(
            writer::OutWriter::WRITE_STDOUT, "",
            get_graph_format(args_info)));
      }

      if (args_info.output_dep_graph_given) {
        analyzers.push_back(new DependencyInferenceAnalyzer(
            writer::OutWriter::WRITE_FILE, args_info.output_dep_graph_orig,
            get_graph_format(args_info)));
      }
    }
  }
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
    exit(1);
  }
  process_args(args_info);
  cmdline_parser_free(&args_info);
  free_argv(argc, c_argv);
  // TODO Create a CLI for the client.
  dr_set_client_name("Client for Detecting Data Races in Files", "");
  dr_printf("Starting the FSRacer Client...\n");
  drmgr_init();
  drwrap_init();
  drsym_init(0);
  dr_register_exit_event(event_exit);
  drmgr_register_module_load_event(module_load_event);
}
