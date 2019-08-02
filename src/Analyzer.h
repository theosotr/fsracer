#ifndef ANALYZER_H
#define ANALYZER_H

#include <fstream>

#include "Operation.h"
#include "OutWriter.h"
#include "Trace.h"


using namespace operation;
using namespace trace;

namespace analyzer {


/**
 * This is the main interface for analyzers.
 *
 * Each analyzer must implement every method responsible for
 * analyzing every node specified in the AST of traces.
 *
 * This is achieved through the visitor design pattern.
 */
class Analyzer {
  public:
    /** Name of analyzer. */
    virtual string GetName();

    /** Analyze a node in the AST of traces. */
    virtual void Analyze(TraceNode *trace_node);
    /** Analyze the whole trace. */
    virtual void AnalyzeTrace(Trace *trace);
    /** Analyze a single block */
    virtual void AnalyzeBlock(Block *block);
    /** Analyze a trace expression. */
    virtual void AnalyzeExpr(Expr *expr);
    /** Analyze the 'submitOp' construct. */
    virtual void AnalyzeSubmitOp(SubmitOp *submit_op);
    /** Analyze the 'execOp' construct. */
    virtual void AnalyzeExecOp(ExecOp *exec_op);
    /** Analyze the 'newEvent' construct. */
    virtual void AnalyzeNewEvent(NewEventExpr *new_ev_expr);
    /** Analyze the 'link' construct. */
    virtual void AnalyzeLink(LinkExpr *link_expr);
    /** Analyze the 'trigger' construct. */
    virtual void AnalyzeTrigger(Trigger *trigger_expr);

    // ----- Methods for analyzing FS-related operations -----
  
    /** Analyze an FS operation. */
    virtual void AnalyzeOperation(Operation *operation);
    /** Analzyze the 'newFd' construct. */
    virtual void AnalyzeNewFd(NewFd *new_fd);
    /** Analyze the 'delFd' construct. */
    virtual void AnalyzeDelFd(DelFd *del_fd);
    /** Analyze the 'hpath' construct. */
    virtual void AnalyzeHpath(Hpath *hpath);
    /** Analyze the 'hpathsym' construct. */
    virtual void AnalyzeHpath(HpathSym *hpathsym);
    /** Analyze the 'link' construct. */
    virtual void AnalyzeLink(Link *link);
    /** Analyze the 'rename' construct. */
    virtual void AnalyzeRename(Rename *rename);
    /** Analyze the 'symlink' construct. */
    virtual void AnalyzeSymlink(Symlink *symlink);

    /**
     * This method dumps the analysis output using the given
     * object responsible for writing the output either to
     * standard output or to a dedicated file.
     */
    virtual void DumpOutput(writer::OutWriter *out);

};


/**
 * This is the DumpAnalyzer.
 *
 * This is a simple analyzer whose job is very simple.
 * It iterates over the generated traces and dumps them
 * either to standard output or to a dedicated file.
 */
class DumpAnalyzer : public Analyzer {
  public:
    DumpAnalyzer():
      trace_count(0),
      operation_count(0),
      block_count(0)
  {  }

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
    void AnalyzeTrigger(Trigger *nested_ev_expr);

    void AnalyzeOperation(Operation *operation);
    void AnalyzeNewFd(NewFd *new_fd) { trace_count++; }
    void AnalyzeDelFd(DelFd *del_fd) { trace_count++; }
    void AnalyzeHpath(Hpath *hpath) { trace_count++; }
    void AnalyzeHpathSym(HpathSym *hpathsym) { trace_count++; }
    void AnalyzeLink(Link *link) { trace_count++; }
    void AnalyzeRename(Rename *rename) { trace_count++; }
    void AnalyzeSymlink(Symlink *symlink) { trace_count++; }

    /** Gets the number of trace entries. */
    size_t GetTraceCount() {
      return trace_count;
    }

    void DumpOutput(writer::OutWriter *out);

  private:
    /// This is the string for holding the generated traces.
    string trace_buf;
    /// This is a counter of trace entries.
    size_t trace_count;
    /// This is a counter of FS operations.
    size_t operation_count;
    /// This is a counter of execution blocks.
    size_t block_count;
};


}


#endif
