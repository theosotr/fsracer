#include "DependencyInferenceAnalyzer.h"
#include "Operation.h"
#include "Trace.h"


using namespace operation;
using namespace trace;

namespace analyzer {


void DependencyInferenceAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DependencyInferenceAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }

  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
}


void DependencyInferenceAnalyzer::AnalyzeBlock(Block *block) {
  if (!block) {
    return;
  }

  current_block = block;
  vector<Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
}


void DependencyInferenceAnalyzer::AnalyzeExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void DependencyInferenceAnalyzer::AnalyzeNewEvent(NewEventExpr *new_event) {
  if (!new_event) {
    return;
  }

  // TODO
}


}
