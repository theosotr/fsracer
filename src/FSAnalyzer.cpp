#include <optional>

#include "FSAnalyzer.h"
#include "Operation.h"
#include "Trace.h"


namespace analyzer {

void FSAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void FSAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }

  main_process = trace->GetThreadId();
  cwd = trace->GetCwd();

  vector<ExecOp*> exec_ops = trace->GetExecOps();
  for (auto const &exec_op : exec_ops) {
    AnalyzeExecOp(exec_op);
  }

  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
  
}


void FSAnalyzer::AnalyzeExecOp(ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  op_table.AddEntry(exec_op->GetId(), exec_op);
}


void FSAnalyzer::AnalyzeBlock(Block *block) {
  if (!block) {
    return;
  }
  current_block = block;
  vector<Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
}


void FSAnalyzer::AnalyzeExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void FSAnalyzer::AnalyzeSubmitOp(SubmitOp *submit_op) {
  if (!submit_op) {
    return;
  }

  optional<ExecOp*> exec_op = op_table.GetValue(
      submit_op->GetId());
  if (!exec_op.has_value()) {
    return;
  }
  vector<Operation*> ops = exec_op.value()->GetOperations();
  for (auto const &op : ops) {
    AnalyzeOperation(op);
  }


}


void FSAnalyzer::AnalyzeOperation(Operation *operation) {
  if (operation) {
    operation->Accept(this);
  }
}


void FSAnalyzer::AnalyzeNewFd(NewFd *new_fd) {
  if (!new_fd) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeDelFd(DelFd *del_fd) {
  if (!del_fd) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeHpath(Hpath *hpath) {
  if (!hpath) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeHpathSym(HpathSym *hpathsym) {
  if (!hpathsym) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeLink(Link *link) {
  if (!link) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeRename(Rename *rename) {
  if (!rename) {
    return;
  }
  // TODO
}


void FSAnalyzer::AnalyzeSymlink(Symlink *symlink) {
  if (!symlink) {
    return;
  }
  // TODO
}


}
