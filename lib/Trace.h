#ifndef TRACE_H
#define TRACE_H


#include <iostream>
#include <optional>
#include <vector>

#include "Operation.h"


#define MAIN_BLOCK 1


using namespace std;
using namespace operation;


namespace analyzer {

class Analyzer;

} // namespace analyzer


namespace trace {


/** A generic class that represents a node of the AST of traces. */
class TraceNode {
  public:
    TraceNode() {  }
    /** Polymorphic destructor of the parent class. */
    virtual ~TraceNode() {  };
    /** This converts the current object to a string. */
    virtual string ToString() const = 0;
    /**
     * This method accepts an analyzer, and delegates the analysis
     * of the current trace node through the visitor design pattern.
     */
    virtual void Accept(analyzer::Analyzer *analyzer) const = 0;
};


/** Class that holds debug information. */
class DebugInfo {
public:
  /** Add a debug information. */
  void AddDebugInfo(string debug);

  /** String representation of an instance of this class. */
  string ToString() const;

private:
  /// A vector of debug info.
  vector<string> debug_info;
};


/** A class that represents a trace expression. */ 
class Expr {
  public:
    Expr() {  }
    /** Polymorphic destructor. */
    virtual ~Expr() {  };
    /** This converts the current object to a string. */
    virtual string ToString() const = 0;
    /**
     * This methods accepts an analyzer that is responsible for
     * analyzing the current expression.
     */
    virtual void Accept(analyzer::Analyzer *analyzer) const = 0;

    /** Get the debug information corresponding to this expression. */
    DebugInfo GetDebugInfo() const {
      return debug_info;
    }

    /** Set the debug information for this expression. */
    void AddDebugInfo(string debug_info_) {
      debug_info.AddDebugInfo(debug_info_);
    }

  private:
    /// Debug information corresponding to this expression.
    DebugInfo debug_info;
};


/** An class that represents an event. */
class Event {
  public:
    /**
     * The different types of an event.
     *
     * An event of an S (Strong) type has the highest priority
     * among all the other events.
     * We follow a first-come, first-served approach for two
     * S events.
     *
     * An event of an M (Medium) type has higher priority of
     * events with type W.
     *
     * An event with type W (Weak) has lower priority
     * than those events that have type S or W.
     * Also, the execution of two events of S type is not non-deterministic,
     * unless their values are the same.
     * If this is the case we follow a FIFO approach.
     *
     * Finally, an event with type EXT (external) is associated with
     * the external environment, so its execution is
     * completely non-determinitstic.
     */
    enum EventType {
      S,
      M,
      W,
      EXT,
      MAIN
    };

    /**
     * Construct a new event with the given type and event value.
     */
    Event(enum EventType event_type_, size_t event_value_):
      event_type(event_type_),
      event_value(event_value_) {  }

    /** Destruct the current event object. */
    Event() {  }

    /** Setter for the event_value field. */
    void SetEventValue(size_t event_value_) {
      event_value = event_value_;
    }

    /** Setter for the event_type field. */
    void SetEventType(enum EventType event_type_) {
      event_type = event_type_;
    }

    /** Getter for the event_type field. */
    enum EventType GetEventType() const {
      return event_type;
    }

    /** Getter for the event_value field. */
    size_t GetEventValue() const {
      return event_value;
    }

    /** Accept an analyzer for processing the current event object. */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current event object. */
    string ToString() const;

  private:
    /// Type of the event.
    enum EventType event_type;

    /**
     * Value of the event.
     *
     * This value is a priority number for events of the same class.
     * Note that this applies only in events of type M.
     * This means that an event with type M and value 1 has higher priority
     * than then event with M and value 2 (1 < 2).
     */
    size_t event_value;

};


/**
 * A class that represents a 'submitOp' expression.
 *
 * This expression indicates that an operation is submitted to
 * be executed either synchronously or asynchronously.
 */
class SubmitOp : public Expr {
  public:
    /**
     * The enumeration indicates the type of the operation.
     * It's either synchronous or asynchronous.
     */
    enum Type {
      ASYNC,
      SYNC
    };

    /** Constructor for creating a new asynchronous operation. */
    SubmitOp(string op_id_, size_t event_id_):
      op_id(op_id_),
      event_id(event_id_),
      type(ASYNC) {  }

    /** Constructor for creating a new synchronous operation. */
    SubmitOp(string op_id_):
      op_id(op_id_),
      type(SYNC) {  }
    
    /** Destructs the current 'submitOp' construct. */
    ~SubmitOp() {  }

    /** Accepts an analyzer to process the current 'submitOp' expression. */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current object. */
    string ToString() const;

    /** Getter of the `id` field. */
    string GetOpId() const {
      return op_id;
    }

    optional<size_t> GetEventId() const {
      return event_id;
    }

    /** Getter of the `type` field. */
    enum Type GetType() const {
      return type;
    }

  private:
    /// Id of the current operation.
    string op_id;

    /**
     * Id of the event corresponding to this operation.
     * If `event_id` is absent, then the operation is synchronous.
     */
    optional<size_t> event_id;

    // Type of the current operation.
    enum Type type;
};


/**
 * This class representa an `execOp` expression.
 *
 * This expression decomposes a high-level operation into a
 * list of FStrace primitives.
 */
class ExecOp : public TraceNode {
  public:
    /** Construct a new 'execOp' expression with the given id. */
    ExecOp(string id_):
      id(id_) {  }

    /** Destructs the current 'execOp' object. */
    ~ExecOp() {
      ClearOperations();
    }

    /** Add an FStrace operation to the current expression .*/
    void AddOperation(Operation *operation) {
      operations.push_back(operation);
    }

    /** Get the list of FStrace operations. */
    vector<const Operation *> GetOperations() const {
      return vector<const Operation *>(operations.begin(), operations.end());
    }

    /** Marks the last inserted operation as failed. */
    void MarkLastOperationFailed();

    /** Getter for the `id` field. */
    string GetId() const {
      return id;
    }

    /**
     * This method accepts an analyzer that is responsible for processing
     * the current 'execOp' primtive.
     */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current expression. */
    string ToString() const;

  private:
    /// Id of the current high-level operation. */
    string id;

    /// Vector of FStrace operation included in the current expression. */
    vector<Operation *> operations;

    /**
     * Cleanup the vector of FStrace operations and
     * deallocate memory properly.
     */
    void ClearOperations();
};


/**
 * This class represents the "newEvent" construct.
 *
 * This expression indicates the creation of a new event in
 * the current execution block.
 */
class NewEventExpr : public Expr {
  public:
    
    /** Creates a 'newEvent' expression with the given id and event type. */
    NewEventExpr(size_t event_id_, Event event_):
      event_id(event_id_),
      event(event_) {  }

    /** Destructs the current object. */
    ~NewEventExpr() {  }

    /** Getter for the 'event_id' field. */
    size_t GetEventId() const {
      return event_id;
    }

    /** Getter for the 'event' field. */
    Event GetEvent() const {
      return event;
    }

    /**
     * This method accepts an analyzer that is responsible to
     * process the current expression.
     */ 
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current expression. */
    string ToString() const;

  private:
    /// Id of the current event.
    size_t event_id;

    /// Description of the current event.
    Event event;
};


/**
 * This class represents the "link" construct.
 *
 * This expression explicitly forms a causal relationship between
 * two events, i.e., the event A triggers the event B.
 */
class LinkExpr : public Expr {
  public:
    
    /**
     * Construct a new `link` expression that forms a causal relationship
     * between the given events.
     */
    LinkExpr(size_t source_ev_, size_t target_ev_):
      source_ev(source_ev_),
      target_ev(target_ev_) {  }

    /** Getter for the `source_ev` field. */
    size_t GetSourceEvent() const {
      return source_ev;
    }

    /** Getter for the `target_ev` field. */
    size_t GetTargetEvent() const {
      return target_ev;
    }

    /**
     * This method accepts an analyzer that is responsible for analyzing
     * the current `link` expression. */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current expression. */
    string ToString() const;

  private:
    /**
     * Id of the source event, i.e., the event that triggers the other one.
     */
    size_t source_ev;
    
    /**
     * If of the target event, i.e., the event that is created
     * by the other one.
     */
    size_t target_ev;
};


/**
 * This expression triggers the execution of an event's callback
 * as part of the execution of the current block.
 */
class Trigger : public Expr {
  public:
    Trigger(size_t event_id_):
      event_id(event_id_) {  }

    /** Getter of the `event_id` field. */
    size_t GetEventId() const {
      return event_id;
    }

    /**
     * This method accepts an analyzer that is responsible for analyzing
     * the current `trigger` expression. */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current expression. */
    string ToString() const;

  private:
    /// The ID of the event whose callback is executed as part of the current
    /// block.
    size_t event_id;

};


/**
 * This class represents an execution block.
 *
 * An execution block indicates the boundaries of the execution of
 * an event.
 */
class Block : public TraceNode {
  public:
    enum Type {
      REG,
      MAIN
    };

    /** Construct an execution block with the specified Id. */
    Block(size_t block_id_):
      block_id(block_id_),
      block_type(REG)
  {  }

    Block(size_t block_id_, enum Type block_type_):
      block_id(block_id_),
      block_type(block_type_) {  }

    /** Destruct the current execution block. */
    ~Block() {
      ClearExprs();
    }

    /**
     * Get the list of expressions triggered by the current execution block.
     */
    vector<const Expr*> GetExprs() const {
      return vector<const Expr*>(exprs.begin(), exprs.end());
    }

    /** Add a new expression to the current execution block .*/
    void AddExpr(Expr *expr) {
      exprs.push_back(expr);
    }

    /** Get the id of the current execution block */
    size_t GetBlockId() const {
      return block_id;
    }

    enum Type GetBlockType() {
      return block_type;
    }

    string GetPrettyBlockId() const;

    bool IsMain() const {
      return block_type == MAIN;
    }

    /** Gets the last expression of the block and remove it. */
    void PopExpr();

    /**
     * This method accepts an analyzer that is responsible for processing the
     * current execution block.
     */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current execution block. */
    string ToString() const;

    /**
     * Get the size of the current block in terms of the number of
     * expressions.
     */
    size_t Size() const;

    /**
     * Set the debug information of the expression located at the given index.
     */
    void SetExprDebugInfo(size_t index, string debug_info);

  private:
    /// The vector of expressions included in the current execution block. */
    vector<Expr*> exprs;

    /// Id of the current execution block. */
    size_t block_id;

    enum Type block_type;

    /// Clear the vector of expressions and deallocate memory properly.
    void ClearExprs();
};


/**
 * This class represents the whole trace generated by
 * the execution of a program.
 *
 * A trace consists of a list of execution blocks,
 * and `execOp` primitives.
 */
class Trace : public TraceNode {
  public:
    
    /** Destruct the current trace. */
    ~Trace() {
      ClearBlocks();
      ClearExecOps();
    }

    /** Get the list of blocks. */
    vector<const Block*> GetBlocks() const {
      return vector<const Block*>(blocks.begin(), blocks.end());
    };

    /** Get the list of `execOp` primitives. */
    vector<const ExecOp*> GetExecOps() const {
      return vector<const ExecOp*>(exec_ops.begin(), exec_ops.end());
    }

    /** Add the given block to the current trace. */
    void AddBlock(Block *block) {
      blocks.push_back(block);
    };

    /** Add the given `execOp` primitive to the current trace. */
    void AddExecOp(ExecOp *exec_op) {
      exec_ops.push_back(exec_op);
    }

    void PopBlock();

    /**
     * Get the id of the main thread of the program associated with
     * the current trace.
     */
    size_t GetThreadId() const {
      return thread_id;
    }

    /** Setter for the `thread_id` field. */
    void SetThreadId(size_t thread_id_) {
      thread_id = thread_id_;
    }

    /**
     * Get the initial working directory of the program associated
     * with the current trace.
     */
    string GetCwd() const {
      return cwd;
    }

    /** Setter for the `cwd` field. */
    void SetCwd(const string cwd_) {
      cwd = cwd_;
    }

    /**
     * This method accepts an analyzer that is responsible for processing
     * the current trace.
     */
    void Accept(analyzer::Analyzer *analyzer) const;

    /** String representation of the current trace. */
    string ToString() const;

  private:
    /// Vector of execution blocks included in the current trace. */
    vector<Block*> blocks;

    /// Vector of `execOp` primitives included in the current trace. */
    vector<ExecOp*> exec_ops;

    /// Id of the main thread of the program. */
    size_t thread_id;

    /**
     * The working directory of the program when the execution
     * starts.
     *
     * Note that it is possible to change the initial
     * working directory of the program.
     * However, this field indicates its initial value.
     */
    string cwd;

    /** Clear the vector of execution blocks. */
    void ClearBlocks();

    /** Clear the vector of `execOps`. */
    void ClearExecOps();
};


} // namespace trace

#endif
