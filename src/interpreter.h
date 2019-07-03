#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <fstream>

#include "operation.h"
#include "trace.h"


using namespace operation;
using namespace trace;

namespace interpreter {


class Interpreter {
  public:
    virtual string GetName();

    virtual void Interpret(TraceNode *trace_node);
    virtual void InterpretTrace(Trace *trace);
    virtual void InterpretBlock(Block *block);
    virtual void InterpretExpr(Expr *expr);
    virtual void InterpretSyncOp(SyncOp *sync_op);
    virtual void InterpretAsyncOp(AsyncOp *async_op);
    virtual void InterpretNewEvent(NewEventExpr *new_ev_expr);
    virtual void InterpretLink(LinkExpr *link_expr);

    //Operations
    virtual void InterpretNewFd(NewFd *new_fd);
    virtual void InterpretDelFd(DelFd *del_fd);
    virtual void InterpretHpath(Hpath *hpath);
    virtual void InterpretHpath(HpathSym *hpathsym);
    virtual void InterpretLink(Link *link);
    virtual void InterpretRename(Rename *rename);
    virtual void InterpretSymlink(Symlink *symlink);
};


class DumpInterpreter : public Interpreter {
  public:
    enum DumpOption {
      FILE_DUMP,
      STDOUT_DUMP
    };

    DumpInterpreter(enum DumpOption dump_option_, string filename_):
      trace_count(0),
      dump_option(dump_option_),
      filename(filename_) {
        SetupOutStream();
      }

    ~DumpInterpreter() {
      ClearOutStream();
    }

    string GetName() {
      return "DumpInterpreter";
    }

    void Interpret(TraceNode *trace_node);
    void InterpretTrace(Trace *trace);
    void InterpretBlock(Block *block);
    void InterpretExpr(Expr *expr);
    void InterpretSyncOp(SyncOp *sync_op);
    void InterpretAsyncOp(AsyncOp *async_op);
    void InterpretNewEvent(NewEventExpr *new_ev_expr);
    void InterpretLink(LinkExpr *link_expr);

    void InterpretNewFd(NewFd *new_fd) {  }
    void InterpretDelFd(DelFd *del_fd) {  }
    void InterpretHpath(Hpath *hpath) {  }
    void InterpretHpath(HpathSym *hpathsym) {  }
    void InterpretLink(Link *link) {  }
    void InterpretRename(Rename *rename) {  }
    void InterpretSymlink(Symlink *symlink) {  }

    enum DumpOption GetDumpOption() {
      return dump_option;
    }

    size_t GetTraceCount() {
      return trace_count;
    }

  private:
    size_t trace_count;
    enum DumpOption dump_option;
    ofstream of;
    string filename;

    void IncrTraceCount() {
      trace_count++;
    }

    void SetupOutStream();
    void ClearOutStream();
    ostream &OutStream();
};


}


#endif
