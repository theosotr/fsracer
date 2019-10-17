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


std::string DumpAnalyzer::GetName() const {
  return "DumpAnalyzer";
}


void DumpAnalyzer::Analyze(const fstrace::TraceNode *trace_node) {
  if (!trace_node) {
    return;
  }
  trace_buf += trace_node->ToString();
  trace_buf += "\n";
}


void DumpAnalyzer::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  out->OutStream() << trace_buf;
}


} // namespace analyzer
