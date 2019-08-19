#ifndef ANALYZER_H
#define ANALYZER_H

#include <fstream>

#include "Operation.h"
#include "OutWriter.h"
#include "Trace.h"
#include "Utils.h"


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
    virtual string GetName() const;

    /** Analyze a node in the AST of traces. */
    virtual void Analyze(const TraceNode *trace_node);
    /** Analyze the whole trace. */
    virtual void AnalyzeTrace(const Trace *trace);
    /** Analyze a single block */
    virtual void AnalyzeBlock(const Block *block);
    /** Analyze a trace expression. */
    virtual void AnalyzeExpr(const Expr *expr);
    /** Analyze the 'submitOp' construct. */
    virtual void AnalyzeSubmitOp(const SubmitOp *submit_op);
    /** Analyze the 'execOp' construct. */
    virtual void AnalyzeExecOp(const ExecOp *exec_op);
    /** Analyze the 'newEvent' construct. */
    virtual void AnalyzeNewEvent(const NewEventExpr *new_ev_expr);
    /** Analyze the 'link' construct. */
    virtual void AnalyzeLink(const LinkExpr *link_expr);
    /** Analyze the 'trigger' construct. */
    virtual void AnalyzeTrigger(const Trigger *trigger_expr);

    // ----- Methods for analyzing FS-related operations -----
  
    /** Analyze an FS operation. */
    virtual void AnalyzeOperation(const Operation *operation);
    /** Analzyze the 'newFd' construct. */
    virtual void AnalyzeNewFd(const NewFd *new_fd);
    /** Analyze the 'delFd' construct. */
    virtual void AnalyzeDelFd(const DelFd *del_fd);
    /** Analyze the 'hpath' construct. */
    virtual void AnalyzeHpath(const Hpath *hpath);
    /** Analyze the 'hpathsym' construct. */
    virtual void AnalyzeHpathSym(const HpathSym *hpathsym);
    /** Analyze the 'link' construct. */
    virtual void AnalyzeLink(const Link *link);
    /** Analyze the 'rename' construct. */
    virtual void AnalyzeRename(const Rename *rename);
    /** Analyze the 'symlink' construct. */
    virtual void AnalyzeSymlink(const Symlink *symlink);

    /**
     * This method dumps the analysis output using the given
     * object responsible for writing the output either to
     * standard output or to a dedicated file.
     */
    virtual void DumpOutput(writer::OutWriter *out) const;

    /** Get the analysis time in milli seconds. */
    double GetAnalysisTime() const {
      return analysis_time.GetTimeMillis();
    }

  protected:
    /// Track analysis time.
    utils::timer analysis_time;

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

    string GetName() const {
      return "DumpAnalyzer";
    }

    void Analyze(const TraceNode *trace_node);
    void AnalyzeTrace(const Trace *trace);
    void AnalyzeBlock(const Block *block);
    void AnalyzeExpr(const Expr *expr);
    void AnalyzeSubmitOp(const SubmitOp *submit_op);
    void AnalyzeExecOp(const ExecOp *exec_op);
    void AnalyzeNewEvent(const NewEventExpr *new_ev_expr);
    void AnalyzeLink(const LinkExpr *link_expr);
    void AnalyzeTrigger(const Trigger *nested_ev_expr);

    void AnalyzeOperation(const Operation *operation);
    void AnalyzeNewFd(const NewFd *new_fd) { trace_count++; }
    void AnalyzeDelFd(const DelFd *del_fd) { trace_count++; }
    void AnalyzeHpath(const Hpath *hpath) { trace_count++; }
    void AnalyzeHpathSym(const HpathSym *hpathsym) { trace_count++; }
    void AnalyzeLink(const Link *link) { trace_count++; }
    void AnalyzeRename(const Rename *rename) { trace_count++; }
    void AnalyzeSymlink(const Symlink *symlink) { trace_count++; }

    /** Gets the number of trace entries. */
    size_t GetTraceCount() const {
      return trace_count;
    }

    void DumpOutput(writer::OutWriter *out) const;

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
