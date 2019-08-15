#include <iostream>
#include <utility>

#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"


#include "Generator.h"

#define CHECK_EXEC_OP                                \
  if (!get_exec_op) {                                \
    return;                                          \
  }                                                  \
  ExecOp *exec_op = get_exec_op(wrapctx, user_data); \
  if (!exec_op) {                                    \
    return;                                          \
  }                                                  \


#define MULTIPATH                                                         \
  generator::Generator *trace_gen = GetTraceGenerator(user_data);         \
  string old_path = (const char *) drwrap_get_arg(wrapctx, old_path_pos); \
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos); \


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
    } else {
      // We now register the address of the function to the function registry.
      // Each entry maps the address of the function to its name.
      void *ptr = (void *) towrap;
      string addr = utils::PtrToString(ptr);
      this->AddFunc(addr, func_name);
    }
  }
}


void Generator::Setup(const module_data_t *mod) {
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


void Generator::PushFunction(string func_name) {
  call_stack.push(func_name);
}


void Generator::PopStack() {
  if (!call_stack.empty()) {
    call_stack.pop();
  }
}


string Generator::TopStack() {
  return call_stack.top();
}


void Generator::AddFunc(string addr, string func_name) {
  funcs[addr] = func_name;
}


string Generator::GetFuncName(string addr) {
  map<string, string>::iterator it = funcs.find(addr);

  if (it != funcs.end()) {
    return it->second;
  }
  return "";
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
DefaultPre(void *wrapctx, OUT void **user_data)
{
  // Push the name of the function we wrap onto the stack.
  void *ptr = drwrap_get_func(wrapctx);
  string addr = utils::PtrToString(ptr);
  generator::Generator *trace_gen = GetTraceGenerator(user_data);
  string func_name = trace_gen->GetFuncName(addr);
  if (func_name != "") {
    trace_gen->PushFunction(func_name);
  }
}


void
DefaultPost(void *wrapctx, void *user_data)
{
  generator::Generator *trace_gen = (generator::Generator *) (user_data);
  trace_gen->PopStack();
}


void
EmitDelFd(void *wrapctx, OUT void **user_data, size_t fd_pos,
          exec_op_t get_exec_op, string op_name)
{
  CHECK_EXEC_OP;
  int fd = (int)(intptr_t) drwrap_get_arg(wrapctx, fd_pos);
  DelFd *delfd = new DelFd(fd);
  delfd->SetActualOpName(op_name);
  exec_op->AddOperation(delfd);
}


void
EmitHpath(void *wrapctx, OUT void **user_data, size_t path_pos,
          Hpath::EffectType effect_type, bool follow_symlink,
          exec_op_t get_exec_op, string op_name)
{
  CHECK_EXEC_OP;
  generator::Generator *generator = GetTraceGenerator(user_data);
  string path = (const char *) drwrap_get_arg(wrapctx, path_pos);
  if (follow_symlink) {
    Hpath *hpath = new Hpath(AT_FDCWD, path, effect_type);
    hpath->SetActualOpName(op_name);
    exec_op->AddOperation(hpath);
  } else {
    HpathSym *hpathsym = new HpathSym(AT_FDCWD, path, effect_type);
    hpathsym->SetActualOpName(op_name);
    exec_op->AddOperation(hpathsym);
  }
}


void
EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
         size_t new_path_pos, exec_op_t get_exec_op, string op_name)
{
  CHECK_EXEC_OP;
  MULTIPATH;
  Link *link = new Link(AT_FDCWD, old_path, AT_FDCWD, new_path);
  link->SetActualOpName(op_name);
  exec_op->AddOperation(link);
}


void
EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
           size_t new_path_pos, exec_op_t get_exec_op, string op_name)
{
  CHECK_EXEC_OP;
  MULTIPATH;
  Rename *rename = new Rename(AT_FDCWD, old_path, AT_FDCWD, new_path);
  rename->SetActualOpName(op_name);
  exec_op->AddOperation(rename);
}


void
EmitSymlink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
            size_t new_path_pos, exec_op_t get_exec_op, string op_name)
{
  CHECK_EXEC_OP;
  MULTIPATH;
  Symlink *symlink = new Symlink(AT_FDCWD, new_path, old_path);
  symlink->SetActualOpName(op_name);
  exec_op->AddOperation(symlink);
}


void MarkOperationStatus(void *wrapctx, void *user_data,
                         exec_op_post_t get_exec_op)
{
  CHECK_EXEC_OP;
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  if (ret_val < 0) {
    Operation *op = exec_op->GetLastOperation();
    if (op) {
      op->MarkFailed();
    }
  }
}


}
