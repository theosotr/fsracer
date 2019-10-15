#ifndef ANALYZER_EXPERIMENTAL_H
#define ANALYZER_EXPERIMENTAL_H

#include <string>

#include "FStrace.h"
#include "OutWriter.h"
#include "Utils.h"


namespace analyzer {


/**
 * This is the main interface for analyzers.
 *
 * Each analyzer must implement every method responsible for
 * analyzing every node specified in the AST of traces.
 *
 * This is achieved through the visitor design pattern.
 */
class AnalyzerExp {
  public:
    virtual ~AnalyzerExp() {  }
    /** Name of analyzer. */
    virtual std::string GetName() const = 0;

    /** Analyze a node in the AST of traces. */
    virtual void Analyze(const fstrace::TraceNode *trace_node);
    virtual void AnalyzeExpr(const fstrace::Expr *expr);
    virtual void AnalyzeConsumes(const fstrace::Consumes *consumes) = 0;
    virtual void AnalyzeProduces(const fstrace::Produces *produces) = 0;
    virtual void AnalyzeNewTask(const fstrace::NewTask *new_task) = 0;
    virtual void AnalyzeDependsOn(const fstrace::DependsOn *depends_on) = 0;
    virtual void AnalyzeExecTask(const fstrace::ExecTask *exec_task) = 0;
    virtual void AnalyzeExecTaskBeg(const fstrace::ExecTaskBeg *exec_task) = 0;
    virtual void AnalyzeSysOp(const fstrace::SysOp *sys_op) = 0;
    virtual void AnalyzeSysOpBeg(const fstrace::SysOpBeg *sys_op) = 0;
    virtual void AnalyzeEnd(const fstrace::End *end) = 0;

    // ----- Methods for analyzing FS-related operations -----
  
    /** Analyze an FS operation. */
    virtual void AnalyzeOperation(const fstrace::Operation *operation);
    /** Analzyze the 'newFd' construct. */
    virtual void AnalyzeNewFd(const fstrace::NewFd *new_fd) = 0;
    /** Analyze the 'delFd' construct. */
    virtual void AnalyzeDelFd(const fstrace::DelFd *del_fd) = 0;
    /** Analyze the 'dupFd' construct. */
    virtual void AnalyzeDupFd(const fstrace::DupFd *dup_fd) = 0;
    /** Analyze the 'hpath' construct. */
    virtual void AnalyzeHpath(const fstrace::Hpath *hpath) = 0;
    /** Analyze the 'hpathsym' construct. */
    virtual void AnalyzeHpathSym(const fstrace::HpathSym *hpathsym) = 0;
    /** Analyze the 'link' construct. */
    virtual void AnalyzeLink(const fstrace::Link *link) = 0;
    /** Analyze the 'rename' construct. */
    virtual void AnalyzeRename(const fstrace::Rename *rename) = 0;
    /** Analyze the 'symlink' construct. */
    virtual void AnalyzeSymlink(const fstrace::Symlink *symlink) = 0;
    /** Analyze the 'newproc' construct. */
    virtual void AnalyzeNewProc(const fstrace::NewProc *new_proc) = 0;
    /** Analyze the 'setcwd' construct. */
    virtual void AnalyzeSetCwd(const fstrace::SetCwd *set_cwd) = 0;
    /** Analyze the 'setcwdfd' construct. */
    virtual void AnalyzeSetCwdFd(const fstrace::SetCwdFd *set_cwdfd) = 0;

    /**
     * This method dumps the analysis output using the given
     * object responsible for writing the output either to
     * standard output or to a dedicated file.
     */
    virtual void DumpOutput(writer::OutWriter *out) const = 0;

    /** Get the analysis time in milli seconds. */
    double GetAnalysisTime() const {
      return analysis_time.GetTimeMillis();
    }

  protected:
    /// Track analysis time.
    utils::timer analysis_time;

};


} // namespace analyzer


#endif
