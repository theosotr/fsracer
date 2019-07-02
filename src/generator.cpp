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

generator::Generator *GetTraceGenerator(void **data) {
  if (!data) {
    return nullptr;
  }

  generator::Generator *trace_gen = (generator::Generator *) *data;
  return trace_gen;
}


}
