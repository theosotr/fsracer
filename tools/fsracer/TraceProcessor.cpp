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
    const ExecTask *exec_task = dynamic_cast<const ExecTask*>(trace_node);
    if (exec_task) {
      ProcessExecTask(exec_task);
      return;
    }
    debug::msg() << trace_node->ToString();
  } else {
    if (in_sysop) {
      debug::msg() << "}";
      in_sysop = false;
      return;
    }
    if (in_exectask) {
      debug::msg() << "}";
      in_exectask = false;
    }
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
    debug::msg() << exec_task->GetHeader() << " {";
  }
}


} // namespace fstrace
