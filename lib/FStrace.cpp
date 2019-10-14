#include "FStrace.h"

#include "AnalyzerExperimental.h"


namespace fstrace {


void DebugInfo::AddDebugInfo(std::string debug) {
  debug_info.push_back(debug);
}


std::string DebugInfo::ToString() const {
  std::string str = "";
  for (auto const &debug : debug_info) {
    str += " !" + debug;
  }
  return str;
}


size_t Task::GetTaskValue() const {
  return task_value;
}


enum Task::TaskType Task::GetTaskType() const {
  return task_type;
}


std::string Task::ToString() const {
  std::string str = std::to_string(task_value);
  switch (task_type) {
    case S:
      return "S " + str;
    case M:
      return "M " + str;
    case W:
      return "W " + str;
    case MAIN:
      return "MAIN";
    default:
      return "EXTERNAL";
  }
}


std::string NewTask::ToString() const {
  DebugInfo debug_info = GetDebugInfo();
  return "newTask " + task_name + " " + task.ToString() + debug_info.ToString();
}


std::string NewTask::GetTaskName() const {
  return task_name;
}


Task NewTask::GetTask() const {
  return task;
}


void NewTask::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeNewTask(this);
}


std::string Consumes::GetTaskName() const {
  return task_name;
}


std::string Consumes::GetObject() const {
  return object;
}


std::string Consumes::ToString() const {
  return "consumes " + task_name + " " + object;
}


void Consumes::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeConsumes(this);
}


std::string Produces::GetTaskName() const {
  return task_name;
}


std::string Produces::GetObject() const {
  return object;
}


std::string Produces::ToString() const {
  return "produces " + task_name + " " + object;
}


void Produces::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeProduces(this);
}


std::string DependsOn::GetTarget() const {
  return target;
}


std::string DependsOn::GetSource() const {
  return source;
}


std::string DependsOn::ToString() const {
  return "dependsOn " + target + " " + source;
}


void DependsOn::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeDependsOn(this);
}


size_t Operation::GetPid() const {
  return pid;
}


void Operation::MarkFailed() {
  failed = true;
}


bool Operation::IsFailed() const {
  return failed;
}


void Operation::SetActualOpName(std::string actual_op_name_) {
  actual_op_name = actual_op_name_;
}


std::string Operation::GetActualOpName() const {
  return actual_op_name;
}


std::string NewFd::GetFilename() const {
  return filename;
}


size_t NewFd::GetDirFd() const {
  return dirfd;
}


int NewFd::GetFd() const {
  return fd;
}


std::string NewFd::GetOpName() const {
  return "newfd";
}


std::string NewFd::ToString() const {
  std::string dirfd_str = DirfdToString(dirfd);
  std::string pid_str = std::to_string(pid) + ", ";
  if (failed) {
    return pid_str + GetOpName() + " " + dirfd_str + " " + filename +
      ACTUAL_NAME + FAILED;
  }
  return pid_str + GetOpName() + " " + dirfd_str + " " + filename +
    " " + std::to_string(fd) + ACTUAL_NAME + FAILED;
};


void NewFd::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeNewFd(this);
}


size_t DelFd::GetFd() const {
  return fd;
}


std::string DelFd::GetOpName() const {
  return "delfd";
}


std::string DelFd::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + std::to_string(fd) +
    ACTUAL_NAME + FAILED;
}


void DelFd::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeDelFd(this);
}


size_t DupFd::GetOldFd() const {
  return old_fd;
}


size_t DupFd::GetNewFd() const {
  return new_fd;
}


std::string DupFd::GetOpName() const {
  return "dupfd";
}


std::string DupFd::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " "  + std::to_string(old_fd) + " " +
    std::to_string(new_fd) + ACTUAL_NAME + FAILED;
};


void DupFd::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeDupFd(this);
}


size_t Hpath::GetDirFd() const {
  return dirfd;
}


std::string Hpath::GetFilename() const {
  return filename;
}


enum Hpath::AccessType Hpath::GetAccessType() const {
  return access_type;
}


std::string Hpath::GetOpName() const {
  return "hpath";
}


std::string Hpath::ToString() const {
  std::string str = DirfdToString(dirfd);
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + str + " " + filename + " " +
    Hpath::AccToString(access_type) + ACTUAL_NAME + FAILED;
};


void Hpath::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeHpath(this);
}


std::string Hpath::AccToString(enum AccessType access) {
  switch (access) {
    case CONSUMED:
      return "consumed";
    case PRODUCED:
      return "produced";
    case EXPUNGED:
      return "expunged";
  }
}


bool Hpath::Consumes(enum AccessType access) {
  switch (access) {
    case CONSUMED:
      return true;
    default:
      return false;
  }
}


std::string HpathSym::GetOpName() const {
  return "hpathsym";
}


void HpathSym::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeHpathSym(this);
}


size_t Link::GetOldDirfd() const {
  return old_dirfd;
}


size_t Link::GetNewDirfd() const {
  return new_dirfd;
}


std::string Link::GetOldPath() const {
  return old_path;
}


std::string Link::GetNewPath() const {
  return new_path;
}


std::string Link::ToString() const {
  std::string old_dirfd_str = DirfdToString(old_dirfd);
  std::string new_dirfd_str = DirfdToString(new_dirfd);
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + old_dirfd_str + " " +
    old_path + " " + new_dirfd_str + " " + new_path + ACTUAL_NAME + FAILED;
};


std::string Link::GetOpName() const {
  return "link";
}


void Link::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeLink(this);
}


enum NewProc::CloneMode NewProc::GetCloneMode() const {
  return clone_mode;
}


size_t NewProc::GetNewProcId() const {
  return pid;
}


std::string NewProc::GetOpName() const {
  return "newproc";
}


std::string NewProc::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  switch (clone_mode) {
    case SHARE_FD:
      return pid_str + GetOpName() + " fd " + std::to_string(new_pid) +
        ACTUAL_NAME + FAILED;
    case SHARE_FS:
      return pid_str + GetOpName() + " fs " + std::to_string(new_pid) +
        ACTUAL_NAME + FAILED;
    case SHARE_BOTH:
      return pid_str + GetOpName() + " fdfs " + std::to_string(new_pid) +
        ACTUAL_NAME + FAILED;
    default:
      return pid_str + GetOpName() + " none " + std::to_string(new_pid) +
        ACTUAL_NAME + FAILED;
  }
}


void NewProc::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeNewProc(this);
}


std::string Rename::GetOpName() const {
  return "rename";
}


void Rename::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeRename(this);
}


std::string SetCwd::GetCwd() const {
  return cwd;
}


std::string SetCwd::GetOpName() const {
  return "setcwd";
}


std::string SetCwd::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + cwd + ACTUAL_NAME + FAILED;
}


void SetCwd::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeSetCwd(this);
}


size_t SetCwdFd::GetFd() const {
  return fd;
}


std::string SetCwdFd::GetOpName() const {
  return "setcwdfd";
}


std::string SetCwdFd::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + std::to_string(fd) +
    ACTUAL_NAME + FAILED;
}


void SetCwdFd::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeSetCwdFd(this);
}


size_t Symlink::GetDirFd() const {
  return dirfd;
}


std::string Symlink::GetFilename() const {
  return filename;
}


std::string Symlink::GetTargetPath() const {
  return target;
}


std::string Symlink::GetOpName() const {
  return "symlink";
}


std::string Symlink::ToString() const {
  std::string pid_str = std::to_string(pid) + ", ";
  return pid_str + GetOpName() + " " + std::to_string(dirfd) + " " +
    filename + " " + target + ACTUAL_NAME + FAILED;
}


void Symlink::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeSymlink(this);
}


void SysOp::AddOperation(Operation *operation) {
  if (operation) {
    operations.push_back(operation);
  }
}


std::vector<const Operation*> SysOp::GetOperations() const {
  return std::vector<const Operation *>(operations.begin(), operations.end());
}


std::string SysOp::GetOpId() const {
  return op_id;
}


std::optional<std::string> SysOp::GetTaskName() const {
  return task_name;
}


std::string SysOp::GetHeader() const {
  switch (op_type) {
    case SYNC:
      return "sysop " + op_id + " SYNC";
    case ASYNC:
      return "sysop " + op_id + " " + task_name.value() + " ASYNC";
  }
}

std::string SysOp::ToString() const {
  std::string str = GetHeader();
  str += "{\n";
  for (auto const &operation : operations) {
    if (!operation) {
      continue;
    }
    str += operation->ToString();
    str += "\n";
  }
  str += "}";
  return str;
}


void SysOp::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeSysOp(this);
}


void ExecTask::AddExpr(Expr *expr) {
  if (expr) {
    exprs.push_back(expr);
  }
}


std::vector<const Expr*> ExecTask::GetExpressions() const {
  return std::vector<const Expr*>(exprs.begin(), exprs.end());
}


std::string ExecTaskBeg::GetTaskName() const {
  return task_name;
}


std::string ExecTaskBeg::ToString() const {
  return "execTask " + task_name + " {";
}


std::string ExecTask::ToString() const {
  std::string str = ExecTaskBeg::ToString();
  str += "\n";
  for (auto const &expr : exprs) {
    if (!expr) {
      continue;
    }
    str += expr->ToString();
    str += "\n";
  }
  str += "}";
  return str;
}


void ExecTask::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeExecTask(this);
}


void ExecTaskBeg::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeExecTaskBeg(this);
}


std::string End::ToString() const {
  return "}";
}


void End::Accept(analyzer::Analyzer *analyzer) const {
  analyzer->AnalyzeEnd(this);
}


} // namespace fstrace
