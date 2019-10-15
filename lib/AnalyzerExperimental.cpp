#include "AnalyzerExperimental.h"


namespace analyzer {


void AnalyzerExp::Analyze(const fstrace::TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void AnalyzerExp::AnalyzeExpr(const fstrace::Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void AnalyzerExp::AnalyzeOperation(const fstrace::Operation *oper) {
  if (oper) {
    oper->Accept(this);
  }
}


} // namespace analyzer
