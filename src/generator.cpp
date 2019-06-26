#include <iostream>

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


static void
wrap_func(const module_data_t *mod, const char *func_name,
          pre_clb_t pre, post_clb_t post)
{
  app_pc towrap = get_pc_by_symbol(mod, func_name);
  if (towrap != NULL) {
    bool wrapped = drwrap_wrap(towrap, pre, post);
    if (!wrapped) {
      dr_fprintf(STDERR,
          "FSRacer Error: Couldn't wrap the %s function\n", func_name);
    }
  }
}

namespace generator {


void Generator::RegisterFunc(const module_data_t *mod, string func_name,
                             pre_clb_t pre , post_clb_t post) {
  wrap_func(mod, func_name.c_str(), pre, post);
}


}
