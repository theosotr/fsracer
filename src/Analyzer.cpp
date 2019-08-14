#include <iostream>
#include <vector>

#include "Analyzer.h"


using namespace operation;
using namespace trace;

namespace analyzer {


void DumpAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    analysis_time.Start();
    trace_node->Accept(this);
    analysis_time.Stop();
  }
}


void DumpAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }
  trace_buf += "!PID: ";
  trace_buf += to_string(trace->GetThreadId());
  trace_buf += "\n";

  trace_buf += "!Working Directory: ";
  trace_buf += trace->GetCwd();
  trace_buf += "\n";
  vector<ExecOp*> exec_ops = trace->GetExecOps();
  operation_count = exec_ops.size();
  for (auto const &exec_op : exec_ops) {
    AnalyzeExecOp(exec_op);
  }
  vector<Block*> blocks = trace->GetBlocks();
  block_count = blocks.size();
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
  trace_count += exprs.size();
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
  trace_count += exec_op->GetOperations().size();
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


void DumpAnalyzer::AnalyzeOperation(Operation *operation) {
  if (operation) {
    operation->Accept(this);
  }
}


void DumpAnalyzer::DumpOutput(writer::OutWriter *out) {
  if (out) {
    string preamble = "";
    preamble += "!Blocks: " + to_string(block_count) + "\n";
    preamble += "!Operations: " + to_string(operation_count) + "\n";
    preamble += "!Trace entries: " + to_string(trace_count) + "\n";
    out->OutStream() << preamble << trace_buf;

    // The same object cannot be used again.
    delete out;
    out = nullptr;
  }
}


}
