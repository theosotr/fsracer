#include <iostream>
#include <vector>

#include "interpreter.h"


using namespace operation;
using namespace trace;

namespace interpreter {


void DumpInterpreter::Interpret(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DumpInterpreter::InterpretTrace(Trace *trace) {
  if (!trace) {
    return;
  }
  cout << "Start Interpreting Trace...\n";
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
  cout << "Begin " << to_string(block->GetBlockId()) << "\n";
  for (auto const &expr : exprs) {
    InterpretExpr(expr);
  }
  cout << "End\n";
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
  cout << sync_op->ToString() << "\n";
}


void DumpInterpreter::InterpretAsyncOp(AsyncOp *async_op) {
  if (!async_op) {
    return;
  }
  cout << async_op->ToString() << "\n";
}


void DumpInterpreter::InterpretNewEvent(NewEventExpr *new_ev_expr) {
  if (!new_ev_expr) {
    return;
  }
  string str = new_ev_expr->ToString();
  cout << str << "\n";
}


void DumpInterpreter::InterpretLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  string str = link_expr->ToString();
  cout << str << "\n";
}


// Operations

void DumpInterpreter::InterpretNewFd(NewFd *new_fd) {
  if (new_fd) {
    cout << new_fd->NewFd::ToString() << "\n";
  }
}


void DumpInterpreter::InterpretDelFd(DelFd *del_fd) {
  if (del_fd) {
    cout << del_fd->DelFd::ToString() << "\n";
  }
}


}
