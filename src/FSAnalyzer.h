#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <experimental/filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "Analyzer.h"
#include "EffectTable.h"
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
  using proc_t = size_t;
  using addr_t = size_t;
  using fd_t = size_t;

  public:
    enum OutFormat {
      JSON,
      CSV
    };

    FSAnalyzer(enum OutFormat out_format_):
      current_block(nullptr),
      main_process(0),
      out_format(out_format_)
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

    const EffectTable &GetFSAccesses() {
      return effect_table;
    }

  private:
    Table<proc_t, inode_t> cwd_table;
    Table<pair<proc_t, fd_t>, inode_key_t> fd_table;
    Table<inode_t, fs::path> symlink_table;
    Table<proc_t, pair<addr_t, addr_t>> proc_table;
    EffectTable effect_table;
    InodeTable inode_table;

    Table<string, ExecOp*> op_table;

    Block *current_block;
    size_t main_process;
    fs::path cwd;
    size_t block_id;

    enum OutFormat out_format;

    void ProcessPathEffect(fs::path p, enum Hpath::EffectType effect);
    optional<fs::path> GetParentDir(size_t dirfd);
    optional<fs::path> GetAbsolutePath(size_t dirfd, fs::path p);
    void UnlinkResource(inode_t inode_p, string basename);

    void DumpJSON(ostream &os);
    void DumpCSV(ostream &os);

};


}
#endif
