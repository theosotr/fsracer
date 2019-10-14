
#include <cstring>
#include <stdio.h>
#include <signal.h>
#include <sstream>

#include "Debug.h"
#include "Utils.h"
#include "TraceProcessor.h"
#include "StreamTraceGenerator.h"



void
sig_handler(int signo)
{
  if (signo == SIGINT) {
    debug::msg() << "Exiting program after SIGINT signal";
    exit(0);
  }
}


int
main(int argc, char **argv)
{
  signal(SIGINT, sig_handler);
  fstrace::TraceProcessor trace_proc;
  fstrace::StreamTraceGenerator trace_gen (argv[1]);
  trace_gen.Start();
  while (trace_gen.HasNext()) {
    fstrace::TraceNode *trace_node = trace_gen.GetNextTrace();
    if (trace_gen.HasFailed()) {
      break;
    }
    trace_proc.ProcessTrace(trace_node);
    if (trace_node) {
      delete trace_node;
    }
  }
  trace_gen.Stop();
  if (trace_gen.HasFailed()) {
    debug::err(trace_gen.GetName()) << trace_gen.GetErr();
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}
