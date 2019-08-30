#ifndef GENERATOR_H
#define GENERATOR_H

#include <iostream>
#include <map>
#include <utility>
#include <stack>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "Utils.h"
#include "Trace.h"
#include "TraceGenerator.h"


using namespace trace;
using namespace std;
using namespace operation;

/// Some type alias for the function pointers corresponding
/// to the wrapper functions of DynamoRIO.
using pre_clb_t = void (*)(void *wrapctx, OUT void **user_data);
using post_clb_t = void (*)(void *wrapctx, void *user_data);


namespace trace_generator {

/**
 * This class represents a trace generator.
 *
 * A trace generator is used to produce traces derived from the
 * execution of a program by wrapping function through DynamoRIO.
 */
class DynamoTraceGenerator : public TraceGenerator {
  public:
    /// This data structures hold the wrappers associated with
    /// every native function name.
    using wrapper_t = map<string, pair<pre_clb_t, post_clb_t>>;

    /** Default Constructor. */
    DynamoTraceGenerator():
      current_block(nullptr),
      event_count(0),
      sync_op_count(0) {
      trace = new Trace();
    }

    /** Deallocate the trace generator. */
    ~DynamoTraceGenerator() {
      if (trace) {
        delete trace; 
      }
    }

    // -------- Operations on the trace generators' store -------------
    
    /** Adds a new key-value entry to the store. */
    void AddToStore(string key, void *value);

    /** Gets the value associated with the given key. */
    void *GetStoreValue(const string &key) const;

    /** Deletes the given key from the store. */
    void DeleteFromStore(const string &key);

    /**
     * Returns value associated with the given key,
     * and then deletes the corresponding entry.
     */
    void *PopFromStore(const string &key);

    // -------- Operations on the functions' stack ----------------
    
    /** Pushes a new function onto the stack. */
    void PushFunction(string func_name);

    /** Pops the top value of the stack. */
    void PopStack();

    /** Gets the function corresponding to the top of the stack. */
    string TopStack() const;

    /**
     * Adds a new entry to the function information store.
     *
     * It associates the given address with the given function name.
     */
    void AddFunc(string addr, string func_name);
    
    /** Get the function name corresponding to the given address. */
    string GetFuncName(const string &addr) const;

    /**
     * Aborts the trace collection and the execution of the program
     * for the given reason and error.
     */
    void AbortWithErr(enum utils::err::ErrType err_type, string errmsg,
                      string location=__builtin_FUNCTION());

    /** Checks whether trace collection has failed. */
    bool HasFailed() const;
    /**
     * Gets the error associated it with the failure of the
     * trace collector.
     */
    utils::err::Error GetErr() const;

    /** Setups the wrapper functions defined by the trace collector. */
    void Setup(const module_data_t *mod);

    /** Start collecting traces. */
    virtual void Start() = 0;

    /** Stop collecting traces. */
    virtual void Stop() = 0;

    /** Gets the wrapper function. */
    virtual wrapper_t GetWrappers() const = 0;

    /** Gets the name of the trace collector. */
    virtual string GetName() const = 0;

    /** Gets the trace collection time in seconds. */
    double GetTraceGenerationTime() const {
      return gen_time.GetTimeSeconds();
    };

    /** Getter of the `trace` field. */
    Trace *GetTrace() {
      return trace;
    }

    /** Getter of the `current_block` field. */
    Block *GetCurrentBlock() {
      return current_block;
    }

    /** Setter of the `current_block` field. */
    void SetCurrentBlock(Block *block) {
      current_block = block;
    }

    /** Increment the number of events. */
    void IncrEventCount() {
      event_count++;
    }

    /** Increment the number of synchronous operations. */
    void IncrSyncOpCount() {
      sync_op_count++;
    }

    /** Get the number of created events. */
    size_t GetEventCount() const {
      return event_count;
    }

    /** Get the number of synchronous operations. */
    size_t GetSyncOpCount() const {
      return sync_op_count;
    }


  private:
    /// Generated trace derived from program's execution.
    Trace *trace;
    /// Pointer to the block that is currently being analyzed.
    Block *current_block;
    /// Number of created events.
    size_t event_count;
    /// Number of synchronous operations.
    size_t sync_op_count;
    /// A key-value data structure used to store temporary
    /// values during trace collection.
    map<string, void*> store;
    /// A callstack.
    stack<string> call_stack;
    /// A data structure that maps addresses to the function they point to.
    map<string, string> funcs;

    /// Field that indicates whether there was an error during
    /// trace collection.
    optional<utils::err::Error> error;

    /// Register the specified function pointers to the function
    /// corresponding to the given name.
    void RegisterFunc(const module_data_t *mod, string func_name,
                      pre_clb_t pre, post_clb_t post);

  protected:
    /// A utility to track trace collection time.
    utils::timer gen_time;
};


} // namespace trace_generator


namespace generator_utils {

using exec_op_t = ExecOp *(*)(void *wrapctx, OUT void **user_data);
using exec_op_post_t = ExecOp *(*)(void *wrapctx, void *user_data);

size_t GetCurrentThread(void *wrapctx);

trace_generator::DynamoTraceGenerator *GetTraceGenerator(void **data);

void DefaultPre(void *wrapctx, OUT void **user_data);

void DefaultPost(void *wrapctx, void *user_data);


template<typename Fn, Fn fn, typename... Args>
void pre_wrap(void *wrapctx, OUT void **user_data) {
  // Call the actual wrapper
  fn(wrapctx, user_data);
  DefaultPre(wrapctx, user_data);
}


template<typename Fn, Fn fn, typename... Args>
void post_wrap(void *wrapctx, void *user_data) {
  // Call the actual wrapper
  fn(wrapctx, user_data);
  DefaultPost(wrapctx, user_data);
}


void EmitDelFd(void *wrapctx, OUT void **user_data, size_t fd_pos,
                exec_op_t get_exec_op,
                string op_name);
void EmitHpath(void *wrapctx, OUT void **user_data,
               size_t path_pos, Hpath::EffectType effect_type,
               bool follow_symlink, exec_op_t get_exec_op,
               string op_name);
void EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
              size_t new_path_pos, exec_op_t get_exec_op,
              string op_name);
void EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
                size_t new_path_pos, exec_op_t get_exec_op,
                string op_name);
void EmitSymlink(void *wrapctx, OUT void **user_data, size_t target_path_pos,
                 size_t new_path_pos, exec_op_t get_exec_op,
                 string op_name);
void MarkOperationStatus(void *wrapctx, void *user_data,
                         exec_op_post_t get_exec_op);

} // namespace generator_utils

#endif
