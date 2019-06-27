#include <iostream>

#include "interpreter.h"
#include "trace.h"


using namespace std;


namespace trace {


string Event::ToString() {
  string str = to_string(event_value);
  switch (event_type) {
    case S:
      return "S " + str;
    case M:
      return "M " + str;
    default:
      return "W " + str;
  }
}


string NewEventExpr::ToString() {
  string str = to_string(event_id);
  return "newEvent " + str + " " + event.ToString();
}

void NewEventExpr::Accept(interpreter::Interpreter *interpreter) {
  interpreter->InterpretNewEvent(this);
}


string LinkExpr::ToString() {
  return "Link " + to_string(source_ev) + " " + to_string(target_ev);
}


void LinkExpr::Accept(interpreter::Interpreter *interpreter) {
  interpreter->InterpretLink(this);
}


void Block::ClearExprs() {
  for (size_t i = 0; i < exprs.size(); i++) {
    delete exprs[i];
  }
  exprs.clear();
}


string Block::ToString() {
  string str = "Begin ";
  str += to_string(block_id);
  str += "\n";

  for (auto const &expr : exprs) {
    str += expr->ToString();
    str += "\n";
  }
  return str + "end";
}


void Block::Accept(interpreter::Interpreter *interpreter) {
  interpreter->InterpretBlock(this);
}


void Trace::ClearBlocks() {
  for (size_t i = 0; i < blocks.size(); i++) {
    delete blocks[i];
  }
  blocks.clear();
}


string Trace::ToString() {
  string str = "";
  for (auto const &block : blocks) {
    str += block->Block::ToString();
    str += "\n";
  }
  return str;
}


void Trace::Accept(interpreter::Interpreter *interpreter) {
  interpreter->InterpretTrace(this);
}


}
