#ifndef GENERATOR_H
#define GENERATOR_H

#include <iostream>
#include <map>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "trace.h"



using namespace trace;
using namespace std;
using namespace operation;


typedef void (*pre_clb_t)(void *wrapctx, OUT void **user_data);
typedef void (*post_clb_t)(void *wrapctx, void *user_data);
typedef map<string, pair<pre_clb_t, post_clb_t>> wrapper_t;



namespace generator {


class Generator {
  public:
    Generator() {
      trace = new Trace();
      current_block = nullptr;
      event_count = 0;
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

    size_t GetEventCount() {
      return event_count;
    }

    void IncrEventCount() {
      event_count++;
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
    virtual void Start(const module_data_t *mod);
    virtual wrapper_t GetWrappers();
    virtual string GetName();


  private:
    Trace *trace;
    Block *current_block;
    ExecOp *exec_op;
    size_t event_count;
    map<string, void*> store;

    void RegisterFunc(const module_data_t *mod, string func_name,
                      pre_clb_t pre, post_clb_t post);
};

}


namespace generator_utils {

typedef ExecOp *(*exec_op_t)(void *wrapctx, OUT void **user_data);

generator::Generator *GetTraceGenerator(void **data);
void EmitDelFd(void *wrapctx, OUT void **user_data, size_t fd_pos,
                exec_op_t get_exec_op);
void EmitHpath(void *wrapctx, OUT void **user_data,
               size_t path_pos, Hpath::EffectType effect_type,
               bool follow_symlink, exec_op_t get_exec_op);
void EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
              size_t new_path_pos, exec_op_t get_exec_op);
void EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
                size_t new_path_pos, exec_op_t get_exec_op);
void EmitSymlink(void *wrapctx, OUT void **user_data, size_t target_path_pos,
                 size_t new_path_pos, exec_op_t get_exec_op); 

}

#endif
