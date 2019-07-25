#ifndef ANALYZER_H
#define ANALYZER_H

#include <fstream>

#include "Operation.h"
#include "OutWriter.h"
#include "Trace.h"


using namespace operation;
using namespace trace;

namespace analyzer {


class Analyzer {
  public:
    Analyzer(enum writer::OutWriter::WriteOption write_option,
             string filename) {
      out = new writer::OutWriter(write_option, filename);
    }

    ~Analyzer() {
      if (out) {
        delete out;
      }
    }
    virtual string GetName();

    virtual void Analyze(TraceNode *trace_node);
    virtual void AnalyzeTrace(Trace *trace);
    virtual void AnalyzeBlock(Block *block);
    virtual void AnalyzeExpr(Expr *expr);
    virtual void AnalyzeSubmitOp(SubmitOp *submit_op);
    virtual void AnalyzeExecOp(ExecOp *exec_op);
    virtual void AnalyzeNewEvent(NewEventExpr *new_ev_expr);
    virtual void AnalyzeLink(LinkExpr *link_expr);
    virtual void AnalyzeContext(Context *context_expr);

    //Operations
    virtual void AnalyzeNewFd(NewFd *new_fd);
    virtual void AnalyzeDelFd(DelFd *del_fd);
    virtual void AnalyzeHpath(Hpath *hpath);
    virtual void AnalyzeHpath(HpathSym *hpathsym);
    virtual void AnalyzeLink(Link *link);
    virtual void AnalyzeRename(Rename *rename);
    virtual void AnalyzeSymlink(Symlink *symlink);

    ostream &GetOutStream() {
      return out->OutStream();
    }

  protected:
    writer::OutWriter *out;
};


class DumpAnalyzer : public Analyzer {
  public:
    DumpAnalyzer(writer::OutWriter::WriteOption write_option, string filename):
      Analyzer(write_option, filename),
      trace_count(0) {  }

    string GetName() {
      return "DumpAnalyzer";
    }

    void Analyze(TraceNode *trace_node);
    void AnalyzeTrace(Trace *trace);
    void AnalyzeBlock(Block *block);
    void AnalyzeExpr(Expr *expr);
    void AnalyzeSubmitOp(SubmitOp *submit_op);
    void AnalyzeExecOp(ExecOp *exec_op);
    void AnalyzeNewEvent(NewEventExpr *new_ev_expr);
    void AnalyzeLink(LinkExpr *link_expr);
    void AnalyzeContext(Context *context_expr);

    void AnalyzeNewFd(NewFd *new_fd) {  }
    void AnalyzeDelFd(DelFd *del_fd) {  }
    void AnalyzeHpath(Hpath *hpath) {  }
    void AnalyzeHpath(HpathSym *hpathsym) {  }
    void AnalyzeLink(Link *link) {  }
    void AnalyzeRename(Rename *rename) {  }
    void AnalyzeSymlink(Symlink *symlink) {  }

    size_t GetTraceCount() {
      return trace_count;
    }

  private:
    size_t trace_count;
    void IncrTraceCount() {
      trace_count++;
    }

};


}


#endif
