#ifndef INTERPRETER_H
#define INTERPRETER_H


#include "trace.h"

using namespace trace;

namespace interpreter {


class Interpreter {
  public:
    virtual void Interpret(TraceNode *trace_node);
    virtual void InterpretTrace(Trace *trace);
    virtual void InterpretBlock(Block *block);
    virtual void InterpretExpr(Expr *expr);
    virtual void InterpretNewEvent(NewEventExpr *new_ev_expr);
    virtual void InterpretLink(LinkExpr *link_expr);
};


class DumpInterpreter : public Interpreter {
  public:
    enum DumpOption {
      FILE_DUMP,
      STDOUT_DUMP
    };

    DumpInterpreter(DumpOption dump_option_) {
      trace_count = 0;
      dump_option = dump_option_;
    }

    void Interpret(TraceNode *trace_node);
    void InterpretTrace(Trace *trace);
    void InterpretBlock(Block *block);
    void InterpretExpr(Expr *expr);
    void InterpretNewEvent(NewEventExpr *new_ev_expr);
    void InterpretLink(LinkExpr *link_expr);

    enum DumpOption GetDumpOption() {
      return dump_option;
    }

    size_t GetTraceCount() {
      return trace_count;
    }

  private:
    size_t trace_count;
    enum DumpOption dump_option;

    void IncrTraceCount() {
      trace_count++;
    }
};


}


#endif
