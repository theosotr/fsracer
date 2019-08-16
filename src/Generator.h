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


using namespace trace;
using namespace std;
using namespace operation;


using pre_clb_t = void (*)(void *wrapctx, OUT void **user_data);
using post_clb_t = void (*)(void *wrapctx, void *user_data);
using wrapper_t = map<string, pair<pre_clb_t, post_clb_t>>;



namespace generator {


class Generator {
  public:
    Generator():
      current_block(nullptr),
      event_count(0),
      sync_op_count(0) {
      trace = new Trace();
    }

    ~Generator() {
      if (trace) {
        delete trace; 
      }
    }

    Trace *GetTrace() {
      return trace;
    }

    Block *GetCurrentBlock() {
      return current_block;
    } 

    map<string, void*> GetStore() {
      return store;
    }

    size_t GetSyncOpCount() {
      return sync_op_count;
    }

    size_t GetEventCount() {
      return event_count;
    }

    void IncrEventCount() {
      event_count++;
    }

    void IncrSyncOpCount() {
      sync_op_count++;
    }

    void SetTrace(Trace *trace_) {
      trace = trace_;
    }

    void SetCurrentBlock(Block *block) {
      current_block = block;
    }

    void AddToStore(string key, void *value);
    void *GetStoreValue(string key);
    void DeleteFromStore(string key);
    void *PopFromStore(string key);
    void PushFunction(string func_name);
    void PopStack();
    string TopStack();
    void AddFunc(string addr, string func_name);
    string GetFuncName(string addr);
    void Setup(const module_data_t *mod);
    void AbortWithErr(enum utils::err::ErrType err_type, string errmsg,
                      string location=__builtin_FUNCTION());
    bool HasFailed();
    utils::err::Error GetErr();

    virtual void Start();
    virtual void Stop();
    virtual wrapper_t GetWrappers();
    virtual string GetName();

    double GetTraceGenerationTime() {
      return gen_time.GetTimeSeconds();
    };


  private:
    Trace *trace;
    Block *current_block;
    ExecOp *exec_op;
    size_t event_count;
    map<string, void*> store;
    size_t sync_op_count;
    stack<string> call_stack;
    map<string, string> funcs;

    optional<utils::err::Error> error;

    void RegisterFunc(const module_data_t *mod, string func_name,
                      pre_clb_t pre, post_clb_t post);

  protected:
    utils::timer gen_time;
};

}


namespace generator_utils {

using exec_op_t = ExecOp *(*)(void *wrapctx, OUT void **user_data);
using exec_op_post_t = ExecOp *(*)(void *wrapctx, void *user_data);

generator::Generator *GetTraceGenerator(void **data);

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

}

#endif
