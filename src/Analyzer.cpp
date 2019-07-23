#include <iostream>
#include <vector>

#include "Analyzer.h"


using namespace operation;
using namespace trace;

namespace analyzer {


void DumpAnalyzer::SetupOutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      of.open(filename);
    default:
      break;
  }
}


void DumpAnalyzer::ClearOutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      of.close();
    default:
      break;
  }
}


ostream &DumpAnalyzer::OutStream() {
  switch (dump_option) {
    case FILE_DUMP:
      return of;
    default:
      return cout;
  }
}


void DumpAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DumpAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }
  vector<ExecOp*> exec_ops = trace->GetExecOps();
  for (auto const &exec_op : exec_ops) {
    AnalyzeExecOp(exec_op);
  }
  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
}


void DumpAnalyzer::AnalyzeBlock(Block *block) {
  if (!block) {
    return;
  }
  vector<Expr*> exprs = block->GetExprs();
  size_t block_id = block->GetBlockId();
  OutStream() << "Begin " <<
    (block_id == MAIN_BLOCK ? "MAIN" : to_string(block_id)) << "\n";
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
  OutStream() << "End\n";
}


void DumpAnalyzer::AnalyzeExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void DumpAnalyzer::AnalyzeSubmitOp(SubmitOp *submit_op) {
  if (!submit_op) {
    return;
  }
  OutStream() << submit_op->ToString() << "\n";
}


void DumpAnalyzer::AnalyzeExecOp(ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  OutStream() << exec_op->ToString() << "\n";
}


void DumpAnalyzer::AnalyzeNewEvent(NewEventExpr *new_ev_expr) {
  if (!new_ev_expr) {
    return;
  }
  string str = new_ev_expr->ToString();
  OutStream() << str << "\n";
}


void DumpAnalyzer::AnalyzeLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  string str = link_expr->ToString();
  OutStream() << str << "\n";
}

}
