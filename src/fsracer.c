#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "state.h"


// TODO:
// * Handle setImmediate
// * Handle nextTick


typedef void (*pre_clb_t)(void *wrapctx, OUT void **user_data);
typedef void (*post_clb_t)(void *wrapctx, void *user_data);


struct State *state;


static void
wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data);

static void
wrap_pre_uv_fs_open(void *wrapcxt, OUT void **user_data);

static void
wrap_pre_uv_fs_unlink(void *wrapcxt, OUT void **user_data);

static void
wrap_pre_emit_before(void *wrapctx, OUT void **user_data);

static void
wrap_pre_emit_after(void *wrapctx, OUT void **user_data);

static void
wrap_pre_emit_init(void *wrapctx, OUT void **user_data);

static void
wrap_pre_start(void *wrapctx, OUT void **user_data);

static void
wrap_pre_timerwrap(void *wrapctx, OUT void **user_data);

static void
wrap_pre_fsreq(void *wrapctx, OUT void **user_data);


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


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
    // Wrappers for the libuv functions
    wrap_func(mod, "uv_fs_access", wrap_pre_uv_fs_access, NULL);
    wrap_func(mod, "uv_fs_open", wrap_pre_uv_fs_open, NULL);
    wrap_func(mod, "uv_fs_unlink", wrap_pre_uv_fs_unlink, NULL);

    // Wrappers for the Node.js functions
    wrap_func(mod, "node::AsyncWrap::EmitBefore", wrap_pre_emit_before, NULL);
    wrap_func(mod, "node::AsyncWrap::EmitAfter", wrap_pre_emit_after, NULL);
    wrap_func(mod, "node::AsyncWrap::EmitAsyncInit", wrap_pre_emit_init, NULL);
    wrap_func(mod, "node::Start", wrap_pre_start, NULL);
    wrap_func(mod, "node::(anonymous namespace)::TimerWrap::New", wrap_pre_timerwrap, NULL);
    wrap_func(mod, "node::fs::NewFSReqWrap", wrap_pre_fsreq, NULL);
}


static void
event_exit(void)
{
    dr_printf("Stopping the FSRacer Client...\n");
    drwrap_exit();
    drmgr_exit();
    drsym_exit();
    close_trace_file(state);
    clear_state(state);
}


DR_EXPORT void
dr_client_main(client_id_t client_id, int argc, const char *argv[])
{
    dr_set_client_name("Client for Detecting Data Races in Files", "");
    dr_printf("Starting the FSRacer Client...\n");
    drmgr_init();
    drwrap_init();
    drsym_init(0);
    dr_register_exit_event(event_exit);
    state = init_state();
    drmgr_register_module_load_event(module_load_event);
    setup_trace_file(state, "fsracer.trace");
}


static void
wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data) {
    const char *path = (char *)drwrap_get_arg(wrapctx, 2);
    void *clb = drwrap_get_arg(wrapctx, 4);

    // If not callback is provided, we presume that the call
    // is synchronous.
    //
    // FIXME: Remove duplicate code.
    if (clb == NULL) {
        write_trace(state, "triggerOpSync (hpath %s consumed)\n", path);
    } else {
        write_trace(state, "triggerOpAsync (hpath %s consumed)\n", path);
    }
}


static void
wrap_pre_uv_fs_open(void *wrapctx, OUT void **user_data)
{
    const char *path = (char *) drwrap_get_arg(wrapctx, 2);
    void *clb = drwrap_get_arg(wrapctx, 5);

    if (clb == NULL) {
        write_trace(state, "triggerOpSync (open %s)\n", path);
    } else {
        write_trace(state, "triggerOpAsync (open %s)\n", path);
    }
}


static void
wrap_pre_uv_fs_unlink(void *wrapctx, OUT void **user_data)
{
    const char *path = (char *) drwrap_get_arg(wrapctx, 2);
    void *clb = drwrap_get_arg(wrapctx, 3);

    if (clb == NULL) {
        write_trace(state, "triggerOpSync (hpath %s expunged)\n", path);
    } else {
        write_trace(state, "triggerOpAsync (hpath %s expunged)\n", path);
    }
}


static void
wrap_pre_emit_before(void *wrapctx, OUT void **user_data)
{
    dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
    if (state->current_ev) {
        write_trace(state, "End %d\n", state->current_ev);
        reset_event(state);
    }
    int async_id = *(double *) ctx->ymm; // xmm0 register
    set_current_ev(state, async_id);
    /* EmitBefore(Environment*, double)
     *
     * Get the value of the second argument that is stored
     * in the SSE registers. */
    write_trace(state, "Start %d\n", async_id);
}


static void
wrap_pre_emit_after(void *wrapctx, OUT void **user_data)
{
    dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
    // EmitAfter(Environment*, double)
    write_trace(state, "End %d\n", (int) *(double *) ctx->ymm);
    reset_event(state);
}


static void
wrap_pre_emit_init(void *wrapctx, OUT void **user_data)
{
    dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
    int async_id = *(double *) ctx->ymm; // xmm0 register
    int trigger_async_id = *((double *) ctx->ymm + 8); // xmm1 register

    if (last_event(state)) {
        size_t size = 10 * sizeof(char);
        char *str = event_to_str(last_event(state), size);
        if (str) {
            write_trace(state, "newEvent %d, %s\n", async_id, str);
            dr_global_free(str, size);
        }
    }
    write_trace(state, "link %d %d\n", trigger_async_id, async_id);
}


static void
wrap_pre_start(void *wrapctx, OUT void **user_data)
{
    enum EventType event_type = S;
    struct Event *event = create_ev(event_type, 0);
    set_current_ev(state, 1);
    set_last_ev(state, event);
    write_trace(state, "Start %d\n", state->current_ev);
}


static void
wrap_pre_timerwrap(void *wrapctx, OUT void **user_data)
{
    enum EventType event_type = W;
    update_or_create_ev(state->last_ev_created, event_type, 1);
}


static void
wrap_pre_fsreq(void *wrapctx, OUT void **user_data)
{
    enum EventType event_type = W;
    if (!last_event(state)) {
        set_last_ev(state, create_ev(event_type, 2));
    } else {
        update_or_create_ev(state->last_ev_created, event_type, 2);
    }
}
