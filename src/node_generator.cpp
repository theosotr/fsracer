#include "generator.h"
#include "node_generator.h"

using namespace generator;
using namespace generator_utils;


static void
wrap_pre_open(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }

  char *path = (char *) drwrap_get_arg(wrapctx, 0);
  trace_gen->AddToStore("open/path", path);
}


static void
wrap_post_open(void *wrapctx, void *user_data)
{
  Generator *trace_gen = (Generator *) user_data;
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }

  string path = (const char *) trace_gen->GetStoreValue("open/path");
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
  Generator *trace_gen = GetTraceGenerator(user_data);
  string path = (const char *) drwrap_get_arg(wrapctx, 2);
  void *clb = drwrap_get_arg(wrapctx, 5);

  NewFd *new_fd = new NewFd(path, "");
  trace_gen->AddToStore("uv_fs_open/op", new_fd);
}


static void
wrap_pre_uv_fs_post_open(void *wrapctx, void *user_data)
{
  Generator *trace_gen = (Generator *) user_data;
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  NewFd *new_fd = (NewFd *) trace_gen->GetStoreValue("uv_fs_open/op");
   
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
  Generator *trace_gen = GetTraceGenerator(user_data);
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

  Generator *trace_gen = GetTraceGenerator(user_data);
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
  Generator *trace_gen = GetTraceGenerator(user_data);
  trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
  trace_gen->SetCurrentBlock(NULL);
}


static void
wrap_pre_emit_init(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
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
  Generator *trace_gen = GetTraceGenerator(user_data);
  // This is the block corresponding to the execution of the top-level
  // code.
  //
  // We set the ID of this block to 0.
  trace_gen->SetCurrentBlock(new Block(0));
  trace_gen->IncrEventCount();
}


static void
wrap_pre_timerwrap(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  trace_gen->IncrEventCount();
  trace_gen->NewLastEvent(Event::W, 1);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(),
      trace_gen->GetLastEvent()));
}


static void
wrap_pre_fsreq(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
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
  Generator *trace_gen = GetTraceGenerator(user_data);
  trace_gen->IncrEventCount();
  trace_gen->NewLastEvent(Event::W, 3);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(),
      trace_gen->GetLastEvent()));
}


namespace generator {

wrapper_t NodeTraceGenerator::GetWrappers() {
  wrapper_t wrappers;

  // system calls
  wrappers["open"] = { wrap_pre_open, wrap_post_open };

  // libuv wrappers
  wrappers["uv_fs_open"] = { wrap_pre_uv_fs_open, wrap_pre_uv_fs_post_open };
  wrappers["uv_fs_close"] = { wrap_pre_uv_fs_close, nullptr };

  // Node wrappers
  wrappers["node::Start"] = { wrap_pre_start, nullptr };
  wrappers["node::Environment::AsyncHooks::push_async_ids"] = { wrap_pre_emit_before, nullptr };
  wrappers["node::AsyncWrap::EmitAfter"] = { wrap_pre_emit_after, nullptr };
  wrappers["node::AsyncWrap::EmitAsyncInit"] = { wrap_pre_emit_init, nullptr };
  wrappers["node::(anonymous namespace)::TimerWrap::now"] = { wrap_pre_timerwrap, nullptr };
  wrappers["node::fs::NewFSReqWrap"] = { wrap_pre_fsreq, nullptr };
  wrappers["node::AsyncWrap::EmitPromiseResolve"] = { wrap_pre_promise_resolve, nullptr };
  wrappers["node::PromiseWrap::PromiseWrap"] = { wrap_pre_promise_wrap, nullptr };
  wrappers["node::AsyncWrap::NewAsyncId"] = { wrap_pre_new_async_id, nullptr };
  return wrappers;
}

}
