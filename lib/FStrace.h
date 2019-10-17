#ifndef FSTRACE_H
#define FSTRACE_H

#include <optional>
#include <string>
#include <vector>


#define AT_FDCWD 0
#define FAILED (failed ? " !failed" : "")
#define ACTUAL_NAME (actual_op_name != "" ? " !" + actual_op_name : "")


namespace analyzer {

class Analyzer;

} // namespace analyzer


namespace fstrace {

/** A generic class that represents a node of the AST of traces. */
class TraceNode {
public:
  /** Polymorphic destructor of the parent class. */
  virtual ~TraceNode() {  };
  /** This converts the current object to a string. */
  virtual std::string ToString() const = 0;
  virtual void Accept(analyzer::Analyzer *a) const = 0;
};


/** Class that holds debug information. */
class DebugInfo {
public:
  /** Add a debug information. */
  void AddDebugInfo(std::string debug);

  /** String representation of an instance of this class. */
  std::string ToString() const;

private:
  /// A vector of debug info.
  std::vector<std::string> debug_info;
};


/** A class that represents a trace expression. */ 
class Expr : public TraceNode {
public:
  Expr() {  }
  /** Polymorphic destructor. */
  virtual ~Expr() {  };
  /** This converts the current object to a string. */
  virtual std::string ToString() const = 0;

  /** Get the debug information corresponding to this expression. */
  DebugInfo GetDebugInfo() const {
    return debug_info;
  }

  /** Set the debug information for this expression. */
  void AddDebugInfo(std::string debug_info_) {
    debug_info.AddDebugInfo(debug_info_);
  }
  virtual void Accept(analyzer::Analyzer *a) const = 0;

private:
  /// Debug information corresponding to this expression.
  DebugInfo debug_info;
};


/** An class that represents an task. */
class Task {
public:
  /**
   * The different types of an task.
   *
   * An task of an S (Strong) type has the highest priority
   * among all the other tasks.
   * We follow a first-come, first-served approach for two
   * S tasks.
   *
   * An task of an M (Medium) type has higher priority of
   * tasks with type W.
   *
   * An task with type W (Weak) has lower priority
   * than those tasks that have type S or W.
   * Also, the execution of two tasks of S type is not non-deterministic,
   * unless their values are the same.
   * If this is the case we follow a FIFO approach.
   *
   * Finally, an task with type EXT (external) is associated with
   * the external environment, so its execution is
   * completely non-determinitstic.
   */
  enum TaskType {
    S,
    M,
    W,
    EXT,
    MAIN
  };

  /**
   * Construct a new task with the given type and task value.
   */
  Task(enum TaskType task_type_, size_t task_value_):
    task_type(task_type_),
    task_value(task_value_) {  }

  /** Destruct the current task object. */
  Task() {  }

  /** Getter for the task_type field. */
  enum TaskType GetTaskType() const;

  /** Getter for the task_value field. */
  size_t GetTaskValue() const;

  /** String representation of the current task object. */
  std::string ToString() const;

private:
  /// Type of the task.
  enum TaskType task_type;

  /**
   * Value of the task.
   *
   * This value is a priority number for tasks of the same class.
   * Note that this applies only in tasks of type M.
   * This means that an task with type M and value 1 has higher priority
   * than then task with M and value 2 (1 < 2).
   */
  size_t task_value;

};


/**
 * This class represents the "newTask" construct.
 *
 * This expression indicates the creation of a new task in
 * the current execution block.
 */
class NewTask : public Expr {
public:
  
  /** Creates a 'newtask' expression with the given id and task type. */
  NewTask(std::string task_name_, Task task_):
    task_name(task_name_),
    task(task_) {  }

  /** Getter for the 'task_name' field. */
  std::string GetTaskName() const;
  Task GetTask() const;

  /** String representation of the current expression. */
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  /// Name of task.
  std::string task_name;

  /// Description of the current task.
  Task task;
};


class Consumes : public Expr {

public:
  Consumes(std::string task_name_, std::string object_):
    task_name(task_name_),
    object(object_) {  }

  std::string ToString() const;
  std::string GetTaskName() const;
  std::string GetObject() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::string task_name;
  std::string object;
};


class Produces : public Expr {

public:
  Produces(std::string task_name_, std::string object_):
    task_name(task_name_),
    object(object_) {  }

  std::string ToString() const;
  std::string GetTaskName() const;
  std::string GetObject() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::string task_name;
  std::string object;
};


class DependsOn : public Expr {
public:
  DependsOn(std::string target_, std::string source_):
    target(target_),
    source(source_) {  }

  std::string GetTarget() const;
  std::string GetSource() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::string target;
  std::string source;

};


inline std::string DirfdToString(size_t dirfd) {
  return dirfd == AT_FDCWD ? "AT_FDCWD" : std::to_string(dirfd);
}


class Operation : public TraceNode {
public:
  Operation(size_t pid_):
    pid(pid_),
    failed(false) {  }

  virtual ~Operation() {  };
  virtual std::string ToString() const = 0;
  virtual std::string GetOpName() const = 0;
  virtual void Accept(analyzer::Analyzer *a) const = 0;

  size_t GetPid() const;
  void MarkFailed();
  bool IsFailed() const;
  void SetActualOpName(std::string actual_op_name_);
  std::string GetActualOpName() const;

protected:
  size_t pid;
  bool failed;
  std::string actual_op_name;
};


class NewFd : public Operation {
public:
  NewFd(size_t pid_, size_t dirfd_, std::string filename_, int fd_):
    Operation(pid_),
    dirfd(dirfd_),
    filename(filename_),
    fd(fd_) { }

  std::string GetFilename() const;
  size_t GetDirFd() const;
  int GetFd() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  size_t dirfd;
  std::string filename;
  int fd;
};


class DelFd : public Operation {
public:
  DelFd(size_t pid_, size_t fd_):
    Operation(pid_),
    fd(fd_) {  }

  size_t GetFd() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  size_t fd;
};


class DupFd : public Operation {
public:
  DupFd(size_t pid_, size_t old_fd_, size_t new_fd_):
    Operation(pid_),
    old_fd(old_fd_),
    new_fd(new_fd_) {  }

  size_t GetOldFd() const;
  size_t GetNewFd() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  size_t old_fd;
  size_t new_fd;
};


class Hpath : public Operation {
public:
  enum AccessType {
    PRODUCED,
    CONSUMED,
    EXPUNGED
  };

  Hpath(size_t pid_, size_t dirfd_, std::string filename_,
        enum AccessType access_type_):
    Operation(pid_),
    dirfd(dirfd_),
    filename(filename_),
    access_type(access_type_) {  }
  ~Hpath() {  }

  size_t GetDirFd() const;
  std::string GetFilename() const;
  enum AccessType GetAccessType() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

  static std::string AccToString(enum AccessType effect);
  static bool Consumes(enum AccessType effect);

protected:
  size_t dirfd;
  std::string filename;
  enum AccessType access_type;

};


class HpathSym : public Hpath {
public:
  HpathSym(size_t pid_, size_t dirfd_, std::string filename_,
           enum AccessType access_type_):
    Hpath(pid_, dirfd_, filename_, access_type_) {  }

  std::string GetOpName() const;
  void Accept(analyzer::Analyzer *a) const;
};


class Link : public Operation {
public:
  Link(size_t pid_, size_t old_dirfd_, std::string old_path_, size_t new_dirfd_,
       std::string new_path_):
    Operation(pid_),
    old_dirfd(old_dirfd_),
    old_path(old_path_),
    new_dirfd(new_dirfd_),
    new_path(new_path_) {  }

  size_t GetOldDirfd() const;
  size_t GetNewDirfd() const;
  std::string GetOldPath() const;
  std::string GetNewPath() const;
  std::string ToString() const;
  std::string GetOpName() const;
  void Accept(analyzer::Analyzer *a) const;

protected:
  size_t old_dirfd;
  std::string old_path;
  size_t new_dirfd;
  std::string new_path;

};


class NewProc : public Operation {
public:
  enum CloneMode {
    SHARE_FS,
    SHARE_FD,
    SHARE_BOTH,
    SHARE_NONE
  };

  NewProc(size_t pid_, enum CloneMode clone_mode_, size_t new_pid_):
    Operation(pid_),
    clone_mode(clone_mode_),
    new_pid(new_pid_) {  }

  enum CloneMode GetCloneMode() const;
  size_t GetNewProcId() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  enum CloneMode clone_mode;
  size_t new_pid;
};


class Rename : public Link {
public:
  Rename(size_t pid_, size_t old_dirfd_, std::string old_path_,
         size_t new_dirfd_, std::string new_path_):
    Link(pid_, old_dirfd_, old_path_, new_dirfd_, new_path_) {  }

  std::string GetOpName() const;
  void Accept(analyzer::Analyzer *a) const;
};


class SetCwd : public Operation {
public:
  SetCwd(size_t pid_, std::string cwd_):
    Operation(pid_),
    cwd(cwd_) {  }

  std::string GetCwd() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::string cwd;
};


class SetCwdFd : public Operation {
public:
  SetCwdFd(size_t pid_, size_t fd_):
    Operation(pid_),
    fd(fd_) { }

  size_t GetFd() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  size_t fd;
};


class Symlink : public Operation {
public:
  Symlink(size_t pid_, size_t dirfd_, std::string filename_,
          std::string target_):
    Operation(pid_),
    dirfd(dirfd_),
    filename(filename_),
    target(target_) {  }

  size_t GetDirFd() const;
  std::string GetFilename() const;
  std::string GetTargetPath() const;
  std::string GetOpName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  size_t dirfd;
  std::string filename;
  std::string target;

};


class SysOpBeg : public Expr {
public:
  enum SysOpType {
    ASYNC,
    SYNC
  };

  SysOpBeg(std::string op_id_):
    op_id(op_id_),
    op_type(SYNC) {  }
  SysOpBeg(std::string op_id_, std::string task_name_):
    op_id(op_id_),
    op_type(ASYNC),
    task_name(task_name_) {  }

  std::string GetOpId() const;
  std::optional<std::string> GetTaskName() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

protected:
  std::string op_id;
  enum SysOpType op_type;
  std::optional<std::string> task_name;
};


class SysOp : public SysOpBeg {
public:
  enum SysOpType {
    ASYNC,
    SYNC
  };

  void AddOperation(Operation *operation);
  std::vector<const Operation*> GetOperations() const;
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::vector<Operation*> operations;
};


class ExecTaskBeg : public Expr {
public:
  ExecTaskBeg(std::string task_name_):
    task_name(task_name_) {  }
  std::string GetTaskName() const;
  std::vector<const Expr*> GetExpressions() const; 
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

protected:
  std::string task_name;
};


class ExecTask : public ExecTaskBeg {
public:
  ExecTask(std::string task_name_):
    ExecTaskBeg(task_name_) {  }

  void AddExpr(Expr *expr);
  std::vector<const Expr*> GetExpressions() const; 
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;

private:
  std::vector<Expr*> exprs;

};


class End : public TraceNode {
public:
  End() {  }
  std::string ToString() const;
  void Accept(analyzer::Analyzer *a) const;
};



} // namespace fstrace


#endif
