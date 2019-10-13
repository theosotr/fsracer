#ifndef TRACE_PROCESSOR_H
#define TRACE_PROCESSOR_H

#include <string>

#include "FStrace.h"

namespace fstrace {


class TraceProcessor {
// TODO
public:
  TraceProcessor():
    in_sysop(false),
    in_exectask(false) {  }
  void ProcessTrace(const TraceNode *trace_node);

private:
  bool in_sysop;
  bool in_exectask;

  void ProcessSysOp(const SysOp *sys_op);
  void ProcessExecTask(const ExecTask *exec_task);
  void AnalyzeTrace();
  void DetectFaults();
};


} // namespace fstrace

#endif
