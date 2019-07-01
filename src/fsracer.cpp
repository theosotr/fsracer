#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"
#include "drsyms.h"

#include "generator.h"
#include "interpreter.h"
#include "operation.h"


using namespace generator;
using namespace trace;
using namespace interpreter;
using namespace operation;


//syscalls wrappers
static void wrap_pre_open(void *wrapctx, OUT void **user_data);
static void wrap_post_open(void *wrapctx, void *user_data);

//libuv wrappers
static void wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data);
static void wrap_pre_uv_fs_open(void *wrapcxt, OUT void **user_data);
static void wrap_pre_uv_fs_post_open(void *wrapcxt, OUT void *user_data);
static void wrap_pre_uv_fs_close(void *wrapcxt, OUT void **user_data);
static void wrap_pre_uv_fs_unlink(void *wrapcxt, OUT void **user_data);

// Node wrappers
static void wrap_pre_emit_before(void *wrapctx, OUT void **user_data);
static void wrap_pre_emit_after(void *wrapctx, OUT void **user_data);
static void wrap_pre_emit_init(void *wrapctx, OUT void **user_data);
static void wrap_pre_start(void *wrapctx, OUT void **user_data);
static void wrap_pre_timerwrap(void *wrapctx, OUT void **user_data);
static void wrap_pre_fsreq(void *wrapctx, OUT void **user_data);
static void wrap_pre_promise_resolve(void *wrapctx, OUT void **user_data);
static void wrap_pre_promise_wrap(void *wrapctx, OUT void **user_data);
static void wrap_pre_new_async_id(void *wrapctx, OUT void **user_data);

// TODO:
// * Handle nextTick
//
NodeTraceGenerator *trace_gen;


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
  trace_gen->GetTrace()->SetThreadId(dr_get_thread_id(drcontext));
  trace_gen->RegisterFunc(mod, "open", wrap_pre_open, wrap_post_open);

  trace_gen->RegisterFunc(mod, "uv_fs_access", wrap_pre_uv_fs_access, NULL);
  trace_gen->RegisterFunc(mod, "uv_fs_open", wrap_pre_uv_fs_open, wrap_pre_uv_fs_post_open);
  trace_gen->RegisterFunc(mod, "uv_fs_close", wrap_pre_uv_fs_close, NULL);
  trace_gen->RegisterFunc(mod, "uv_fs_unlink", wrap_pre_uv_fs_unlink, NULL);

  trace_gen->RegisterFunc(mod, "node::Environment::AsyncHooks::push_async_ids", wrap_pre_emit_before, NULL);
  trace_gen->RegisterFunc(mod, "node::AsyncWrap::EmitAfter", wrap_pre_emit_after, NULL);
  trace_gen->RegisterFunc(mod, "node::AsyncWrap::EmitAsyncInit", wrap_pre_emit_init, NULL);
  trace_gen->RegisterFunc(mod, "node::Start", wrap_pre_start, NULL);
  trace_gen->RegisterFunc(mod, "node::(anonymous namespace)::TimerWrap::Now", wrap_pre_timerwrap, NULL);
  trace_gen->RegisterFunc(mod, "node::fs::NewFSReqWrap", wrap_pre_fsreq, NULL);
  trace_gen->RegisterFunc(mod, "node::AsyncWrap::EmitPromiseResolve", wrap_pre_promise_resolve, NULL);
  trace_gen->RegisterFunc(mod, "node::PromiseWrap::PromiseWrap", wrap_pre_promise_wrap, NULL);
  trace_gen->RegisterFunc(mod, "node::AsyncWrap::NewAsyncId", wrap_pre_new_async_id, NULL);
}


static void
event_exit(void)
{
  dr_printf("Stopping the FSRacer Client...\n");
  drwrap_exit();
  drmgr_exit();
  drsym_exit();
  DumpInterpreter *dump_interpreter = new DumpInterpreter(
      DumpInterpreter::STDOUT_DUMP);
  dump_interpreter->Interpret(trace_gen->GetTrace());
  delete dump_interpreter;
  delete trace_gen;
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
  drmgr_register_module_load_event(module_load_event);
  trace_gen = new NodeTraceGenerator();
}


static void
wrap_pre_open(void *wrapctx, OUT void **user_data)
{
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }

  char *path = (char *) drwrap_get_arg(wrapctx, 0);
  *user_data = (void *) path;
}


static void
wrap_post_open(void *wrapctx, void *user_data)
{
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }
  string path = (const char *) user_data;
  string key = "newFd " + path;

  // We search whether there is an incomplete operation named `newFd`
  // that affects the same path as `open`.
  Operation *operation = trace_gen->GetIncompleteOp(key);
  if (!operation) {
    // We did not find any operation; so we return.
    return;
  }

  // We set the `fd` value of the found `newFd` operation.
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  NewFd *new_fd = dynamic_cast<NewFd*>(operation);
  new_fd->SetFd(to_string(ret_val));
}


static void
wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data) {
}


static void
wrap_pre_uv_fs_open(void *wrapctx, OUT void **user_data)
{
  string path = (const char *) drwrap_get_arg(wrapctx, 2);
  void *clb = drwrap_get_arg(wrapctx, 5);

  NewFd *new_fd = new NewFd(path, "");
  *user_data = (void *) new_fd;
}


static void
wrap_pre_uv_fs_post_open(void *wrapctx, void *user_data)
{
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  NewFd *new_fd = (NewFd *) user_data;
   
  SyncOp *op;
  if (ret_val == 0) {
    // If the return value of open is equal to zero,
    // we have an asynchronous open;
    string key = new_fd->ToString(); 
    trace_gen->AddIncompleteOp(key, new_fd);
    op = new AsyncOp(new_fd, trace_gen->GetLastEventId());
  } else {
    new_fd->SetFd(to_string(ret_val));
    op = new SyncOp(new_fd);
  }
  trace_gen->GetCurrentBlock()->AddExpr(op);
}


static void
wrap_pre_uv_fs_close(void *wrapctx, OUT void **user_data)
{
  int fd = (int)(ptr_int_t) drwrap_get_arg(wrapctx, 2);
  void *clb = drwrap_get_arg(wrapctx, 3);

  DelFd *del_fd = new DelFd(to_string(fd));
  if (clb == NULL) {
    trace_gen->GetCurrentBlock()->AddExpr(new SyncOp(del_fd));
  } else {
    trace_gen->GetCurrentBlock()->AddExpr(
        new AsyncOp(del_fd, trace_gen->GetLastEventId()));
  }
}


static void
wrap_pre_uv_fs_unlink(void *wrapctx, OUT void **user_data)
{
  //const char *path = (char *) drwrap_get_arg(wrapctx, 2);
  //void *clb = drwrap_get_arg(wrapctx, 3);

  //if (clb == NULL) {
  //  trace_gen->EmitTriggerOpSync("hpath expunged", 0);
  //} else {
  //  trace_gen->EmitTriggerOpAsync("hpath expunged", 0);
  //}
}


static void
wrap_pre_emit_before(void *wrapctx, OUT void **user_data)
{
  /* node::Environment::AsyncHooks::push_async_ids(double, double)
   *
   * Get the value of the second argument that is stored
   * in the SSE registers. */
  dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  int async_id = *(double *) ctx->ymm; // xmm0 register
  int trigger_async_id = *((double *) ctx->ymm + 8); // xmm1 register

  if (async_id == 0 || async_id == 1) {
    return;
  }


  if (trace_gen->GetCurrentBlock()) {
    // If this values does not point to NULL, we can infer
    // that we did not close the previous block.
    //
    // So we do it right now.
    trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
    trace_gen->SetCurrentBlock(NULL);
  }
  // This hook occurs just before the execution of an event-related callback.
  trace_gen->SetCurrentBlock(new Block(async_id));
}


static void
wrap_pre_emit_after(void *wrapctx, OUT void **user_data)
{
  trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
  trace_gen->SetCurrentBlock(NULL);
}


static void
wrap_pre_emit_init(void *wrapctx, OUT void **user_data)
{
  dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  int async_id = *(double *) ctx->ymm; // xmm0 register
  int trigger_async_id = *((double *) ctx->ymm + 8); // xmm1 register
  trace_gen->IncrEventCount();
  if (trace_gen->IsEventPending() && trace_gen->GetCurrentBlock()) {
    trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
        async_id, trace_gen->GetLastEvent()));
    trace_gen->SetLastEventId(async_id);
  }
}


static void
wrap_pre_start(void *wrapctx, OUT void **user_data)
{
  trace_gen->SetCurrentBlock(new Block(trace_gen->GetEventCount()));
}


static void
wrap_pre_timerwrap(void *wrapctx, OUT void **user_data)
{
  trace_gen->IncrEventCount();
  trace_gen->NewLastEvent(Event::W, 1);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(),
      trace_gen->GetLastEvent()));
}


static void
wrap_pre_fsreq(void *wrapctx, OUT void **user_data)
{
  trace_gen->NewLastEvent(Event::W, 2);
}


static void
wrap_pre_promise_resolve(void *wrapctx, OUT void **user_data)
{
  // dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  // int async_id = *(double *) ctx->ymm; // xmm0 register

  // trace_gen->ConstructSEvent();
  // trace_gen->EmitNewEventTrace(async_id);
  // trace_gen->IncrEventCount();
}


static void
wrap_pre_promise_wrap(void *wrapctx, OUT void **user_data)
{
  // TODO: revisit. This might not be needed.
}


static void
wrap_pre_new_async_id(void *wrapctx, OUT void **user_data)
{
  trace_gen->IncrEventCount();
  trace_gen->NewLastEvent(Event::W, 3);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(),
      trace_gen->GetLastEvent()));
}
