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


}
