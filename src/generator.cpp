#include <iostream>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"


#include "generator.h"


/**
 * This function retrieves the entry point a function.
 *
 * It looks up the entry point by symbol.
 * Initially, it lookups the dynamic symbol table through the invocation
 * of `dr_get_prc_address()`.
 *
 * However, the documentation states, global functions and variables
 * in an executable are not exported by default in the dynamic symbol table.
 *
 * For that purpose, we use the "drsym" extension to locate symbols
 * by searcing the debug symbol table.
 */
static app_pc
get_pc_by_symbol(const module_data_t *mod, const char *symbol)
{
  if (mod == NULL || symbol == NULL)
    return NULL;

  // Try to find the symbol in the dynamic symbol table.
  app_pc pc = (app_pc) dr_get_proc_address(mod->handle, symbol);
  if (pc != NULL) {
    return pc;
  } else {
    size_t offset;
    // Search symbol by omitting its parameters' signature.
    drsym_error_t err =
        drsym_lookup_symbol(mod->full_path, symbol, &offset,
                            DRSYM_DEMANGLE);
    if (err == DRSYM_SUCCESS) {
      pc = mod->start + offset;
      return pc;
    } else {
      return NULL;
    }
  }
}


namespace generator {


void Generator::RegisterFunc(const module_data_t *mod, string func_name,
                             pre_clb_t pre , post_clb_t post) {
  app_pc towrap = get_pc_by_symbol(mod, func_name.c_str());
  void *user_data  = this;
  if (towrap != NULL) {
    bool wrapped = drwrap_wrap_ex(towrap, pre, post, user_data,
        DRWRAP_FLAGS_NONE|DRWRAP_CALLCONV_AMD64);
    if (!wrapped) {
      dr_fprintf(STDERR,
          "FSRacer Error: Couldn't wrap the %s function\n", func_name);
    }
  }
}


void Generator::Start(const module_data_t *mod) {
  for (pair<string, pair<pre_clb_t, post_clb_t>> entry : GetWrappers()) {
    string func_name = entry.first;
    pair<pre_clb_t, post_clb_t> func_pair = entry.second;
    RegisterFunc(mod, func_name, func_pair.first, func_pair.second);
  }
}


void Generator::AddToStore(string key, void *value) {
  store[key] = value;
}


void *Generator::GetStoreValue(string key) {
  map<string, void*>::iterator it;
  it = store.find(key);
  if (it == store.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}


void Generator::DeleteFromStore(string key) {
  PopFromStore(key);
}


void *Generator::PopFromStore(string key) {
  map<string, void*>::iterator it = store.find(key);
  void *value = nullptr;
  if (it != store.end()) {
    value = it->second;
    store.erase(it);
  }
  return value;
}


}


namespace generator_utils {

generator::Generator *
GetTraceGenerator(void **data)
{
  if (!data) {
    return nullptr;
  }

  generator::Generator *trace_gen = (generator::Generator *) *data;
  return trace_gen;
}


void
EmitDelFd(void *wrapctx, OUT void **user_data, size_t fd_pos,
          exec_op_t get_exec_op)
{
  if (!get_exec_op) {
    return;
  }
  ExecOp *exec_op = get_exec_op(wrapctx, user_data);
  if (!exec_op) {
    return;
  }
  int fd = (int)(intptr_t) drwrap_get_arg(wrapctx, fd_pos);
  exec_op->AddOperation(new DelFd(fd));
}


void
EmitHpath(void *wrapctx, OUT void **user_data, size_t path_pos,
          Hpath::EffectType effect_type, bool follow_symlink,
          exec_op_t get_exec_op)
{
  if (!get_exec_op) {
    return;
  }
  ExecOp *exec_op = get_exec_op(wrapctx, user_data);
  if (!exec_op) {
    return;
  }
  generator::Generator *generator = GetTraceGenerator(user_data);
  string path = (const char *) drwrap_get_arg(wrapctx, path_pos);
  if (follow_symlink) {
    exec_op->AddOperation(new Hpath(AT_FDCWD, path, effect_type));
  } else {
    exec_op->AddOperation(new HpathSym(AT_FDCWD, path, effect_type));
  }
}


void
EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
         size_t new_path_pos, exec_op_t get_exec_op)
{
  if (!get_exec_op) {
    return;
  }
  ExecOp *exec_op = get_exec_op(wrapctx, user_data);
  if (!exec_op) {
    return;
  }

  generator::Generator *trace_gen = GetTraceGenerator(user_data);
  string old_path = (const char *) drwrap_get_arg(wrapctx, old_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  exec_op->AddOperation(new Link(AT_FDCWD, old_path, AT_FDCWD, new_path));
}


void
EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
           size_t new_path_pos, exec_op_t get_exec_op)
{
  if (!get_exec_op) {
    return;
  }
  ExecOp *exec_op = get_exec_op(wrapctx, user_data);
  if (!exec_op) {
    return;
  }

  generator::Generator *trace_gen = GetTraceGenerator(user_data);
  string old_path = (const char *) drwrap_get_arg(wrapctx, old_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  exec_op->AddOperation(new Rename(AT_FDCWD, old_path, AT_FDCWD, new_path));
}


void
EmitSymlink(void *wrapctx, OUT void **user_data, size_t target_path_pos,
            size_t new_path_pos, exec_op_t get_exec_op)
{
  if (!get_exec_op) {
    return;
  }
  ExecOp *exec_op = get_exec_op(wrapctx, user_data);
  if (!exec_op) {
    return;
  }

  generator::Generator *trace_gen = GetTraceGenerator(user_data);
  string target_path = (const char *) drwrap_get_arg(wrapctx, target_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  exec_op->AddOperation(new Symlink(AT_FDCWD, new_path, target_path));
}

}
