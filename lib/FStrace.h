#ifndef FSTRACE_H
#define FSTRACE_H

#include <string>
#include <vector>


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

  /** Destructs the current object. */
  ~NewTask() {  }

  /** Getter for the 'task_name' field. */
  std::string GetTaskName() const;
  Task GetTask() const;

  /** String representation of the current expression. */
  std::string ToString() const;

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

private:
  std::string target;
  std::string source;

};

} // namespace fstrace


#endif
