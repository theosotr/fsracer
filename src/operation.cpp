#include "interpreter.h"
#include "operation.h"

namespace operation {


void NewFd::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretNewFd(this);
  }
};


string NewFd::ToString() {
  string str = GetOpName() + " " + path_name;
  if (fd == "")
    return str;
  str += (" " + fd);
  return str;
}


void DelFd::Accept(interpreter::Interpreter *interpreter) {
  if (interpreter) {
    interpreter->InterpretDelFd(this);
  }
};


string DelFd::ToString() {
  return GetOpName() + " " + fd;
}


}
