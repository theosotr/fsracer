#ifndef OPERATION_H
#define OPERATION_H

#include <iostream>


using namespace std;

namespace interpreter {
  class Interpreter;
}


namespace operation {

class Operation {

  public:
    virtual ~Operation() {  };
    virtual void Accept(interpreter::Interpreter *interpreter);
    virtual string ToString();
    virtual string GetOpName();
    string GetReturnValue() { return return_value; };

  private:
    string return_value; 
};


class NewFd : public Operation {
  public:
    NewFd(string path_name_, string fd_):
      path_name(path_name_),
      fd(fd_) { }

    ~NewFd() {  }

    void Accept(interpreter::Interpreter *interpreter);
    string ToString();
    string GetOpName() {
      return "newFd";
    }
    void SetFd(string fd_) {
      fd = fd_;
    }

  private:
    string path_name;
    string fd;
};


class DelFd : public Operation {
  public:
    DelFd(string fd_):
      fd(fd_) {  }

    ~DelFd() {  }

    void Accept(interpreter::Interpreter *interpreter);
    string ToString();
    string GetOpName() {
      return "delFd";
    }

  private:
    string fd;
};


}

#endif
