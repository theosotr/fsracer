#include "Analyzer.h"


namespace analyzer {


void Analyzer::Analyze(const fstrace::TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void Analyzer::AnalyzeExpr(const fstrace::Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void Analyzer::AnalyzeOperation(const fstrace::Operation *oper) {
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
