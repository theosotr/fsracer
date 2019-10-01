#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <experimental/filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

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

  using proc_t = size_t;
  using addr_t = size_t;
  using fd_t = size_t;

  public:
    enum OutFormat {
      JSON,
      CSV
    };

    struct FSAccess {
      size_t event_id;
      enum Hpath::EffectType effect_type;
      DebugInfo debug_info;
      string operation_name;

      FSAccess() {  }

      FSAccess(size_t event_id_, enum Hpath::EffectType effect_type_,
               DebugInfo debug_info_, string operation_name_):
        event_id(event_id_),
        effect_type(effect_type_),
        debug_info(debug_info_),
        operation_name(operation_name_) {  }
    };

    using fs_accesses_table_t = Table<fs::path, vector<FSAccess>>;

    FSAnalyzer(enum OutFormat out_format_):
      current_block(nullptr),
      main_process(0),
      out_format(out_format_)
  {  }

    string GetName() const {
      return "FSAnalyzer";
    }

    void Analyze(const TraceNode *trace_node);
    void AnalyzeTrace(const Trace *trace);
    void AnalyzeBlock(const Block *block);
    void AnalyzeExpr(const Expr *expr);
    void AnalyzeSubmitOp(const SubmitOp *submit_op);
    void AnalyzeExecOp(const ExecOp *exec_op);
    void AnalyzeNewEvent(const NewEventExpr *new_ev_expr);
    void AnalyzeLink(const LinkExpr *link_expr) {  };
    void AnalyzeTrigger(const Trigger *nested_ev_expr) {  };

    void AnalyzeOperation(const Operation *operation);
    void AnalyzeNewFd(const NewFd *new_fd);
    void AnalyzeDelFd(const DelFd *del_fd);
    void AnalyzeHpath(const Hpath *hpath);
    void AnalyzeHpathSym(const HpathSym *hpathsym);
    void AnalyzeLink(const Link *link);
    void AnalyzeRename(const Rename *rename);
    void AnalyzeSymlink(const Symlink *symlink);

    void DumpOutput(writer::OutWriter *out) const;

    const fs_accesses_table_t &GetFSAccesses() const {
      return effect_table;
    }

  private:
    Table<proc_t, inode_t> cwd_table;
    Table<pair<proc_t, fd_t>, inode_key_t> fd_table;
    Table<inode_t, fs::path> symlink_table;
    Table<proc_t, pair<addr_t, addr_t>> proc_table;
    InodeTable inode_table;

    fs_accesses_table_t effect_table;
    Table<pair<fs::path, size_t>, FSAccess> block_accesses;

    Table<string, const ExecOp*> op_table;
    Table<size_t, const NewEventExpr*> event_info;

    const Block *current_block;
    size_t main_process;
    fs::path cwd;
    size_t block_id;

    enum OutFormat out_format;

    void ProcessPathEffect(fs::path p, enum Hpath::EffectType effect,
                           string operation_name);
    optional<fs::path> GetParentDir(size_t dirfd) const;
    optional<fs::path> GetAbsolutePath(size_t dirfd, fs::path p) const;
    void UnlinkResource(inode_t inode_p, string basename);

    void DumpJSON(ostream &os) const;
    void DumpCSV(ostream &os) const;
    void AddPathEffect(const fs::path &p, FSAccess fs_access);
};


} // namespace analyzer

#endif
