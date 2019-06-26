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
  if (!event) {
    return "";
  }
  string str = to_string(event_id);
  return "newEvent " + str + " " + event->Event::ToString();
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
