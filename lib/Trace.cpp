#include "assert.h"
#include <iostream>

#include "Analyzer.h"
#include "Trace.h"


using namespace std;


namespace trace {


string Event::ToString() const {
  string str = to_string(event_value);
  switch (event_type) {
    case S:
      return "S " + str;
    case M:
      return "M " + str;
    case W:
      return "W " + str;
    case MAIN:
      return "MAIN";
    default:
      return "EXTERNAL";
  }
}


string SubmitOp::ToString() const {
  string type_str = "";
  switch (type) {
    case ASYNC:
      type_str = "ASYNC";
      break;
    case SYNC:
      type_str = "SYNC";
  }
  DebugInfo debug_info = GetDebugInfo();
  if (event_id.has_value()) {
    return "submitOp "  + op_id + " " + to_string(event_id.value())
      + " " + type_str + debug_info.ToString();
  }
  return "submitOp " + op_id + " " + type_str + debug_info.ToString();
}


void SubmitOp::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeSubmitOp(this);
}


void ExecOp::MarkLastOperationFailed() {
  if (operations.empty()) {
    return;
  }

  Operation *op = *(operations.end() - 1);
  if (op) {
    op->MarkFailed();
  }
}


void ExecOp::ClearOperations() {
  for (Operation *operation : operations) {
    delete operation;
  }
  operations.clear();
}


string ExecOp::ToString() const {
  string str = "Operation " + id + " do\n";
  for (Operation *operation : operations) {
    str += operation->ToString();
    str += "\n";
  }
  str += "done";
  return str;
}


void ExecOp::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeExecOp(this);
}


void DebugInfo::AddDebugInfo(string debug) {
  debug_info.push_back(debug);
}


string DebugInfo::ToString() const {
  string str = "";
  for (auto const &debug : debug_info) {
    str += " !" + debug;
  }
  return str;
}


string NewEventExpr::ToString() const {
  string str = to_string(event_id);
  DebugInfo debug_info = GetDebugInfo();
  return "newEvent " + str + " " + event.ToString() + debug_info.ToString();
}


void NewEventExpr::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeNewEvent(this);
}


string LinkExpr::ToString() const {
  return "link " + to_string(source_ev) + " " + to_string(target_ev);
}


void LinkExpr::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeLink(this);
}


string Trigger::ToString() const {
  return "trigger " + to_string(event_id);
}


void Trigger::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeTrigger(this);
}


string Block::GetPrettyBlockId() {
  switch (block_type) {
    case REG:
      return "REG_" + to_string(block_id);
    case MAIN:
      return "MAIN_" + to_string(block_id);
  }
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


size_t Block::Size() const {
  return exprs.size();
}


void Block::SetExprDebugInfo(size_t index, string debug_info) {
  assert(index < exprs.size());
  if (exprs[index]) {
    exprs[index]->AddDebugInfo(debug_info);
  }
}


string Block::ToString() const {
  string str = "Begin ";
  str += block_type == MAIN ? "MAIN_" + to_string(block_id)
    : to_string(block_id);
  str += "\n";

  for (auto const &expr : exprs) {
    str += expr->ToString();
    str += "\n";
  }
  return str + "end";
}


void Block::Accept(analyzer::Analyzer *analyzer) const {
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


void Trace::PopBlock() {
  if (blocks.empty()) {
    return;
  }
  vector<Block *>::iterator it = blocks.end() - 1;
  delete *it;
  blocks.erase(it);
}


string Trace::ToString() const {
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


void Trace::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeTrace(this);
}


}
