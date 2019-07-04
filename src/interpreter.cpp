#include <iostream>
#include <vector>

#include "interpreter.h"


using namespace operation;
using namespace trace;

namespace interpreter {


void DumpInterpreter::SetupOutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      of.open(filename);
    default:
      break;
  }
}


void DumpInterpreter::ClearOutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      of.close();
    default:
      break;
  }
}


ostream &DumpInterpreter::OutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      return of;
    default:
      return cout;
  }
}


void DumpInterpreter::Interpret(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DumpInterpreter::InterpretTrace(Trace *trace) {
  if (!trace) {
    return;
  }
  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    InterpretBlock(block);
  }
}


void DumpInterpreter::InterpretBlock(Block *block) {
  if (!block) {
    return;
  }
  vector<Expr*> exprs = block->GetExprs();
  OutStream() << "Begin " << to_string(block->GetBlockId()) << "\n";
  for (auto const &expr : exprs) {
    InterpretExpr(expr);
  }
  OutStream() << "End\n";
}


void DumpInterpreter::InterpretExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void DumpInterpreter::InterpretSyncOp(SyncOp *sync_op) {
  if (!sync_op) {
    return;
  }
  OutStream() << sync_op->ToString() << "\n";
}


void DumpInterpreter::InterpretAsyncOp(AsyncOp *async_op) {
  if (!async_op) {
    return;
  }
  OutStream() << async_op->ToString() << "\n";
}


void DumpInterpreter::InterpretSubmitOp(SubmitOp *submit_op) {
  if (!submit_op) {
    return;
  }
  OutStream() << submit_op->ToString() << "\n";
}


void DumpInterpreter::InterpretExecOp(ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  OutStream() << exec_op->ToString() << "\n";
}


void DumpInterpreter::InterpretNewEvent(NewEventExpr *new_ev_expr) {
  if (!new_ev_expr) {
    return;
  }
  string str = new_ev_expr->ToString();
  OutStream() << str << "\n";
}


void DumpInterpreter::InterpretLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  string str = link_expr->ToString();
  OutStream() << str << "\n";
}

}
