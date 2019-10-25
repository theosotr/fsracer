#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <experimental/filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <set>
#include <vector>

#include "Analyzer.h"
#include "InodeTable.h"
#include "Table.h"


namespace fs = experimental::filesystem;


namespace analyzer {


class FSAnalyzer : public analyzer::Analyzer {

using proc_t = size_t;
using addr_t = size_t;
using fd_t = size_t;
using inode_t = table::inode_t;
using inode_key_t = table::inode_key_t;

public:
  enum OutFormat {
    JSON,
    CSV
  };

  struct FSAccess {
    std::string task_name;
    enum fstrace::Hpath::AccessType access_type;
    fstrace::DebugInfo debug_info;
    std::string operation_name;

    FSAccess() {  }

    FSAccess(std::string task_name_,
             enum fstrace::Hpath::AccessType access_type_,
             fstrace::DebugInfo debug_info_, std::string operation_name_):
      task_name(task_name_),
      access_type(access_type_),
      debug_info(debug_info_),
      operation_name(operation_name_) {  }
  };

  using fs_accesses_table_t = table::Table<fs::path, std::vector<FSAccess>>;

  FSAnalyzer(enum OutFormat out_format_):
    working_dir(""),
    in_sysop(false),
    out_format(out_format_) {  }

  std::string GetName() const;
  void AnalyzeConsumes(const fstrace::Consumes *consumes) {  }
  void AnalyzeProduces(const fstrace::Produces *produces) {  }
  void AnalyzeNewTask(const fstrace::NewTask *new_task) {  }
  void AnalyzeDependsOn(const fstrace::DependsOn *depends_on) {  }
  void AnalyzeExecTask(const fstrace::ExecTask *exec_task) {  }
  void AnalyzeExecTaskBeg(const fstrace::ExecTaskBeg *exec_task);
  void AnalyzeSysOp(const fstrace::SysOp *sys_op) {  }
  void AnalyzeSysOpBeg(const fstrace::SysOpBeg *sys_op);
  void AnalyzeEnd(const fstrace::End *end);

  void AnalyzeNewFd(const fstrace::NewFd *new_fd);
  void AnalyzeDelFd(const fstrace::DelFd *del_fd);
  void AnalyzeDupFd(const fstrace::DupFd *dup_fd);
  void AnalyzeHpath(const fstrace::Hpath *hpath);
  void AnalyzeHpathSym(const fstrace::HpathSym *hpathsym);
  void AnalyzeLink(const fstrace::Link *link);
  void AnalyzeRename(const fstrace::Rename *rename);
  void AnalyzeSymlink(const fstrace::Symlink *symlink);
  void AnalyzeNewProc(const fstrace::NewProc *new_proc);
  void AnalyzeSetCwd(const fstrace::SetCwd *set_cwd);
  void AnalyzeSetCwdFd(const fstrace::SetCwdFd *set_cwdfd);

  void DumpOutput(writer::OutWriter *out) const;
  fs_accesses_table_t GetFSAccesses() const;
  std::string GetWorkingDir() const;
  std::set<fs::path> GetDirectories() const;

private:
  table::Table<proc_t, inode_t> cwd_table;
  table::Table<std::pair<proc_t, fd_t>, inode_key_t> fd_table;
  table::Table<inode_t, fs::path> symlink_table;
  table::Table<proc_t, std::pair<addr_t, addr_t>> proc_table;
  table::InodeTable inode_table;
  mutable fs_accesses_table_t effect_table;
  table::Table<std::pair<fs::path, std::string>, FSAccess> task_accesses;
  std::set<fs::path> dirs;

  std::optional<std::string> current_task;
  std::string working_dir;
  bool in_sysop;
  enum OutFormat out_format;

  void ProcessPathEffect(fs::path p, enum fstrace::Hpath::AccessType access,
                         std::string operation_name);
  std::optional<fs::path> GetParentDir(size_t dirfd, size_t pid) const;
  std::optional<fs::path> GetAbsolutePath(size_t dirfd, fs::path p,
                                          size_t pid) const;
  void UnlinkResource(inode_t inode_p, std::string basename);

  void DumpJSON(ostream &os) const;
  void DumpCSV(ostream &os) const;
  void AddPathEffect(const fs::path &p, FSAccess fs_access);
  void UpdateAccessTable() const;
};


} // namespace analyzer

#endif
