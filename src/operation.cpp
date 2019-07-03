#include "interpreter.h"
#include "operation.h"

namespace operation {


void NewFd::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretNewFd(this);
  }
};


void DelFd::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretDelFd(this);
  }
};


void Hpath::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretHpath(this);
  }
};


void HpathSym::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretHpath(this);
  }
};


void Link::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretLink(this);
  }
};


}
