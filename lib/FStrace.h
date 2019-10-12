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


} // namespace fstrace


#endif
