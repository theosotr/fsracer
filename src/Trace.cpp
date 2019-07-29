#include <iostream>

#include "Analyzer.h"
#include "Trace.h"


using namespace std;


namespace trace {


string Event::ToString() {
  string str = to_string(event_value);
  switch (event_type) {
    case S:
      return "S " + str;
    case M:
      return "M " + str;
    case W:
      return "W " + str;
    default:
      return "EXTERNAL";
  }
}


string SubmitOp::ToString() {
  switch (type) {
    case ASYNC:
      return "SubmitOp "  + id + " " + name + " ASYNC";
    default:
      return "SubmitOp "  + id + " " + name + " SYNC";
  }
}


void SubmitOp::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeSubmitOp(this);
}


void ExecOp::ClearOperations() {
  for (Operation *operation : operations) {
    delete operation;
  }
  operations.clear();
}


string ExecOp::ToString() {
  string str = "Operation " + id + " do\n";
  for (Operation *operation : operations) {
    str += operation->ToString();
    str += "\n";
  }
  str += "done";
  return str;
}


void ExecOp::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeExecOp(this);
}


string NewEventExpr::ToString() {
  string str = to_string(event_id);
  return "newEvent " + str + " " + event.ToString();
}


void NewEventExpr::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeNewEvent(this);
}


string LinkExpr::ToString() {
  return "Link " + to_string(source_ev) + " " + to_string(target_ev);
}


void LinkExpr::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeLink(this);
}


string Trigger::ToString() {
  return "trigger " + to_string(event_id);
}


void Trigger::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeTrigger(this);
}


void Block::PopExpr() {
  if (exprs.empty()) {
    return;
  }
  vector<Expr *>::iterator it = exprs.end() - 1;
  delete *it;
  exprs.erase(it);
}


void Block::ClearExprs() {
  for (size_t i = 0; i < exprs.size(); i++) {
    delete exprs[i];
  }
  exprs.clear();
}


string Block::ToString() {
  string str = "Begin ";
  str += block_id == MAIN_BLOCK ? "MAIN" : to_string(block_id);
  str += "\n";

  for (auto const &expr : exprs) {
    str += expr->ToString();
    str += "\n";
  }
  return str + "end";
}


void Block::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeBlock(this);
}


void Trace::ClearBlocks() {
  for (size_t i = 0; i < blocks.size(); i++) {
    delete blocks[i];
  }
  blocks.clear();
}


void Trace::ClearExecOps() {
  for (size_t i = 0; i < exec_ops.size(); i++) {
    delete exec_ops[i];
  }
  exec_ops.clear();
}


string Trace::ToString() {
  string str = "";
  for (auto const &exec_op : exec_ops) {
    str += exec_op->ExecOp::ToString();
    str += "\n";
  }
  for (auto const &block : blocks) {
    str += block->Block::ToString();
    str += "\n";
  }
  return str;
}


void Trace::Accept(analyzer::Analyzer *analyzer) {
  analyzer->AnalyzeTrace(this);
}


}
