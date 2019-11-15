#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <set>
#include <vector>

#include "Analyzer.h"
#include "InodeTable.h"
#include "Table.h"


namespace fs = std::filesystem;


namespace analyzer {


/**
 * Computes information about every file, including file type (directory,
 * or regular file), and which tasks access every file and how.
 */
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

  /**
   * Describes a single file access.
   */
  struct FSAccess {
    /// Name of the task that accesses that file.
    std::string task_name;
    /// The type of file access (e.g., consumed, produced, expunged)
    enum fstrace::Hpath::AccessType access_type;
    /// An array of debug information
    fstrace::DebugInfo debug_info;
    /**
     * The name of the actual operation (e.g., the name of system call).
     * This is used for debugging purposes.
     */
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

  enum FileType {
    DIRECTORY,
    REGULAR_FILE,
  };

  /**
   * Contains information about a single file including its type,
   * and a vector of all its accesses.
   */
  struct FileInfo {
    /// Path to file.
    fs::path p;
    /// File type (i.e., directory or regular file).
    enum FileType file_type;
    /// It contains which tasks access this file and how.
    std::vector<FSAccess> file_accesses;

    FileInfo() {  }
    FileInfo(fs::path p_, enum FileType file_type_):
      p(p_),
      file_type(file_type_) {  }

    /** True if this file is a directory. */
    bool IsDir() const;
    /** Adds a new file access to the vector of accesses. */
    void AddFileAccess(FSAccess file_access);
  };

  using file_info_t = table::Table<fs::path, FileInfo>;

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
  file_info_t GetFileInfo() const;
  std::string GetWorkingDir() const;

private:
  table::Table<proc_t, inode_t> cwd_table;
  table::Table<std::pair<proc_t, fd_t>, inode_key_t> fd_table;
  table::Table<inode_t, fs::path> symlink_table;
  table::Table<proc_t, std::pair<addr_t, addr_t>> proc_table;
  table::InodeTable inode_table;
  mutable file_info_t file_info;
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

  void DumpJSON(std::ostream &os) const;
  void DumpCSV(std::ostream &os) const;
  void AddPathEffect(const fs::path &p, FSAccess fs_access);
  void UpdateAccessTable() const;
};


} // namespace analyzer

#endif
