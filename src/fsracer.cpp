#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "Analyzer.h"
#include "NodeGenerator.h"


using namespace generator;
using namespace analyzer;


// TODO:
// * Handle nextTick
//
static Generator *trace_gen;
bool module_loaded = false;


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
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
event_exit(void)
{
  cout << trace_gen->GetName() << ": Trace collected\n";
  trace_gen->Stop();
  drwrap_exit();
  drmgr_exit();
  drsym_exit();
  DumpAnalyzer *dump_analyzer = new DumpAnalyzer(
      DumpAnalyzer::STDOUT_DUMP, "");

  // TODO: Currently the analysis of traces is done offline
  // (after the execution of the program).
  //
  // Add support for both offline and online trace analysis.
  cout << dump_analyzer->GetName() << ": Start analyzing traces...\n";
  dump_analyzer->Analyze(trace_gen->GetTrace());
  delete dump_analyzer;
  delete trace_gen;
  cout << dump_analyzer->GetName() << ": Analysis is done\n";
}


DR_EXPORT void
dr_client_main(client_id_t client_id, int argc, const char *argv[])
{
  dr_set_client_name("Client for Detecting Data Races in Files", "");
  dr_printf("Starting the FSRacer Client...\n");
  drmgr_init();
  drwrap_init();
  drsym_init(0);
  dr_register_exit_event(event_exit);
  drmgr_register_module_load_event(module_load_event);
  trace_gen = new NodeTraceGenerator();
}
