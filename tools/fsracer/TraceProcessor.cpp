#include <assert.h>

#include "Debug.h"
#include "TraceProcessor.h"
#include "Utils.h"


namespace fstrace {


void TraceProcessor::ProcessTrace(const TraceNode *trace_node) {
  if (trace_node) {
    const SysOp *sysop = dynamic_cast<const SysOp*>(trace_node);
    if (sysop) {
      ProcessSysOp(sysop);
      return;
    }
    const End *end = dynamic_cast<const End*>(trace_node);
    if (end) {
      debug::msg() << end->ToString();
      if (in_sysop) {
        in_sysop = false;
      }
      return;
    }
    debug::msg() << trace_node->ToString();
  }
}


void TraceProcessor::ProcessSysOp(const SysOp *sysop) {
  if (sysop) {
    in_sysop = true;
    debug::msg() << sysop->GetHeader() << " {";
  }
}

void TraceProcessor::ProcessExecTask(const ExecTask *exec_task) {
  if (exec_task) {
    assert(!in_sysop);
    in_exectask = true;
    debug::msg() << exec_task->ToString();
  }
}


} // namespace fstrace
