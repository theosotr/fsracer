#include <iostream>
#include <vector>

#include "Analyzer.h"


using namespace operation;
using namespace trace;

namespace analyzer {


void DumpAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DumpAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }
  trace_buf += "!PID: ";
  trace_buf += to_string(trace->GetThreadId());
  trace_buf += "\n";
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
  trace_buf += "Begin ";
  trace_buf += (block_id == MAIN_BLOCK ? "MAIN" : to_string(block_id));
  trace_buf += "\n";
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
  trace_buf += "End\n";
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
  trace_buf += submit_op->ToString();
  trace_buf += "\n";
}


void DumpAnalyzer::AnalyzeExecOp(ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  trace_buf += exec_op->ToString();
  trace_buf += "\n";
}


void DumpAnalyzer::AnalyzeNewEvent(NewEventExpr *new_ev_expr) {
  if (!new_ev_expr) {
    return;
  }
  string str = new_ev_expr->ToString();
  trace_buf += str;
  trace_buf += "\n";
}


void DumpAnalyzer::AnalyzeLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  string str = link_expr->ToString();
  trace_buf += str;
  trace_buf += "\n";
}


void DumpAnalyzer::AnalyzeTrigger(Trigger *trigger_expr) {
  if (!trigger_expr) {
    return;
  }
  trace_buf += trigger_expr->ToString();
  trace_buf += "\n";
}


void DumpAnalyzer::DumpOutput(writer::OutWriter *out) {
  if (out) {
    out->OutStream() << trace_buf;

    // The same object cannot be used again.
    delete out;
    out = nullptr;
  }
}


}
