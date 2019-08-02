#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <experimental/filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "Analyzer.h"
#include "InodeTable.h"
#include "Operation.h"
#include "Table.h"
#include "Trace.h"


using namespace std;
using namespace analyzer;
using namespace table;
using namespace trace;

namespace fs = experimental::filesystem;


namespace analyzer {


class FSAnalyzer : public Analyzer {
  typedef size_t proc_t;
  typedef size_t addr_t;
  typedef size_t fd_t;

  public:
    FSAnalyzer():
      current_block(nullptr),
      main_process(0)
  {  }

    string GetName() {
      return "FSAnalyzer";
    }

    void Analyze(TraceNode *trace_node);
    void AnalyzeTrace(Trace *trace);
    void AnalyzeBlock(Block *block);
    void AnalyzeExpr(Expr *expr);
    void AnalyzeSubmitOp(SubmitOp *submit_op);
    void AnalyzeExecOp(ExecOp *exec_op);
    void AnalyzeNewEvent(NewEventExpr *new_ev_expr) {  };
    void AnalyzeLink(LinkExpr *link_expr) {  };
    void AnalyzeTrigger(Trigger *nested_ev_expr) {  };

    void AnalyzeOperation(Operation *operation);
    void AnalyzeNewFd(NewFd *new_fd);
    void AnalyzeDelFd(DelFd *del_fd);
    void AnalyzeHpath(Hpath *hpath);
    void AnalyzeHpathSym(HpathSym *hpathsym);
    void AnalyzeLink(Link *link);
    void AnalyzeRename(Rename *rename);
    void AnalyzeSymlink(Symlink *symlink);

    void DumpOutput(writer::OutWriter *out);

  private:
    Table<addr_t, fs::path> cwd_table;
    Table<pair<addr_t, fd_t>, inode_t> fd_table;
    Table<inode_t, fs::path> symlink_table;
    Table<proc_t, pair<addr_t, addr_t>> proc_table;
    InodeTable inode_table;

    Table<string, ExecOp*> op_table;

    Block *current_block;
    size_t main_process;
    fs::path cwd;

};


}
#endif
