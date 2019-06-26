#include <iostream>
#include <vector>

#include "interpreter.h"


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


}
