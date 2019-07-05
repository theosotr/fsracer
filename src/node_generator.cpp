#include "generator.h"
#include "node_generator.h"
#include "utils.h"

using namespace generator;
using namespace generator_utils;
using namespace generator_keys;


namespace node_utils {


void
AddSubmitOp(void *wrapctx, OUT void **user_data, const string op_name,
            size_t async_pos)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  // First, we get the pointer that refers to the `uv_fs_t` struct
  // of the libuv library.
  //
  // This pointer holds all the necessary information for executing
  // the (asynchronous) operation.
  void *ptr = drwrap_get_arg(wrapctx, 1);
  void *clb = drwrap_get_arg(wrapctx, async_pos);

  string id;
  if (clb == nullptr) {
    // This operation is synchronous.
    // TODO: Generate unique ids for synchronous operations.
    trace_gen->IncrSyncOpCount();
    id = "sync_" + to_string(trace_gen->GetSyncOpCount());
  } else {
    int *event_ptr = (int *) trace_gen->GetStoreValue(
        FUNC_ARGS + "wrap_pre_emit_init");
    id = "async_" + to_string(*event_ptr);
    delete event_ptr;
  }
  // We use the address that this pointer points to as an indicator
  // of the event associated with this operation.
  //
  // In this way, we are able to retrieve the `event_id` related to
  // the execution of the operation.
  //
  // Note that at this point, the operation is not executed by libuv.
  // Instread, it just submits this operation to the worker pool
  // and it is going to be executed in the future.
  string addr = utils::PtrToString(ptr);
  string key = OPERATIONS + addr;
  void *id_ptr = static_cast<void*>(new string(id));
  trace_gen->AddToStore(key, (void *) id_ptr);

  // It's time to add the `submitOp` expression to the current block.
  enum SubmitOp::Type type = clb == nullptr ?
    SubmitOp::SYNC : SubmitOp::ASYNC;
  SubmitOp *submit_op = new SubmitOp(id, op_name, type);
  trace_gen->GetCurrentBlock()->AddExpr(submit_op);
}


}


ExecOp *
get_exec_op(void *wrapctx, OUT void **user_data)
{

  Generator *trace_gen = GetTraceGenerator(user_data);
  size_t thread_id = utils::GetCurrentThread(wrapctx);
  return (ExecOp *) trace_gen->GetStoreValue(
      THREADS + to_string(thread_id));
}


// Wrappers for system calls.
static void
wrap_pre_access(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_chmod(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_chown(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_close(void *wrapctx, OUT void **user_data)
{
  EmitDelFd(wrapctx, user_data, 0, get_exec_op);
}


static void
wrap_pre_lchown(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, false, get_exec_op);
}


static void
wrap_pre_lstat(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, false, get_exec_op);
}


static void
wrap_pre_link(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, false, get_exec_op);
  EmitHpath(wrapctx, user_data, 1, Hpath::PRODUCED, false, get_exec_op);
  EmitLink(wrapctx, user_data, 0, 1, get_exec_op);
}


static void
wrap_pre_mkdir(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::PRODUCED, false, get_exec_op);
}


static void
wrap_pre_open(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  void *drcontext = drwrap_get_drcontext(wrapctx);
  char *path = (char *) drwrap_get_arg(wrapctx, 0);
  int flags = (int)(intptr_t) drwrap_get_arg(wrapctx, 1);

  // We get the two least significant bits of the flags value in order
  // to determine the mode of `open` (i.e., read-only, write-only,
  // read-write). 
  int mode = flags & 3;
  if (mode == 0) {
    // It's read-only.
    EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
  } else {
    // It's either write-only or read-write.
    EmitHpath(wrapctx, user_data, 0, Hpath::PRODUCED, true, get_exec_op);
  }
  string key = FUNC_ARGS + "open";
  trace_gen->AddToStore(key, path);
}


static void
wrap_post_open(void *wrapctx, void *user_data)
{
  Generator *trace_gen = (Generator *) user_data;
  string path = (const char *) trace_gen->PopFromStore(FUNC_ARGS + "open");
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  size_t thread_id = utils::GetCurrentThread(wrapctx);
  ExecOp *exec_op = (ExecOp *) trace_gen->GetStoreValue(
      THREADS + to_string(thread_id));
  if (!exec_op) {
    return;
  }
  exec_op->AddOperation(new NewFd(path, ret_val));
}


static void
wrap_pre_readlink(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_realpath(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_rename(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::EXPUNGED, true, get_exec_op);
  EmitHpath(wrapctx, user_data, 0, Hpath::PRODUCED, true, get_exec_op);
  EmitRename(wrapctx, user_data, 0, 1, get_exec_op);
}


static void
wrap_pre_rmdir(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::EXPUNGED, true, get_exec_op);
}


static void
wrap_pre_stat(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_symlink(void *wrapctx, OUT void **user_data)
{
  EmitSymlink(wrapctx, user_data, 0, 1, get_exec_op);
}


static void
wrap_pre_unlink(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::EXPUNGED, true, get_exec_op);
}


static void
wrap_pre_utime(void *wrapctx, OUT void **user_data)
{
  EmitHpath(wrapctx, user_data, 0, Hpath::CONSUMED, true, get_exec_op);
}


static void
wrap_pre_uv_fs_done(void *wrapctx, OUT void **user_data)
{

  Generator *trace_gen = GetTraceGenerator(user_data);
  char *ptr = (char *) drwrap_get_arg(wrapctx, 0);
  // This is implementation specific.
  //
  // The argument of the `uv__fs_work` function is a pointer
  // of a `uv__work` struct.
  //
  // This pointer is one of the fields that is included inside
  // the `uv_fs_t` struct.
  // The following pointer arithmetic retrieves the pointer
  // that refers to the beginning of that struct where
  // the argument of this function is a member.
  void *uv_fs_t_ptr = ptr - WORKER_OFFSET;
  string addr = utils::PtrToString(uv_fs_t_ptr);
  void *value = trace_gen->PopFromStore(
      THREAD_OPERATIONS + addr);
  if (!value) {
    return;
  }
  int *thread_id = (int *) value;
  trace_gen->DeleteFromStore(THREADS + to_string(*thread_id));
  delete thread_id;
}


static void
wrap_pre_uv_fs_work(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  size_t thread_id = utils::GetCurrentThread(wrapctx);
  char *ptr = (char *) drwrap_get_arg(wrapctx, 0);
  // See the comment inside the `wrap_pre_uv_fs_done` function.
  void *uv_fs_t_ptr = ptr - WORKER_OFFSET;
  string addr = utils::PtrToString(uv_fs_t_ptr);
  string key = OPERATIONS + addr;
  void *value = trace_gen->PopFromStore(key);
  if (!value) {
    return;
  }
  string *op_ptr = static_cast<string*>(value);
  ExecOp *exec_op = new ExecOp(*op_ptr);
  delete op_ptr;
  trace_gen->AddToStore(THREADS + to_string(thread_id), (void *) exec_op);

  int *thread_ptr = new int (thread_id);
  trace_gen->AddToStore(THREAD_OPERATIONS + addr,
                        (void *) thread_ptr);
  trace_gen->GetTrace()->AddExecOp(exec_op);
}


static void
wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "access", 4);
}


static void
wrap_pre_uv_fs_chmod(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "chmod", 4);
}


static void
wrap_pre_uv_fs_chown(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "chown", 5);
}


static void
wrap_pre_uv_fs_copyfile(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "copyFile", 5);
}


static void
wrap_pre_uv_fs_close(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "close", 3);
}



static void
wrap_pre_uv_fs_lchown(void *wrapctx, OUT void **user_data)
{

  node_utils::AddSubmitOp(wrapctx, user_data, "lchown", 5);
}


static void
wrap_pre_uv_fs_lstat(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "lstat", 3);
}


static void
wrap_pre_uv_fs_link(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "link", 4);
}


static void
wrap_pre_uv_fs_mkdir(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "mkdir", 4);
}


static void
wrap_pre_uv_fs_open(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "open", 5);
}


static void
wrap_pre_uv_fs_opendir(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "opendir", 3);
}


static void
wrap_pre_uv_fs_readlink(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "readlink", 3);
}


static void
wrap_pre_uv_fs_realpath(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "realpath", 3);
}


static void
wrap_pre_uv_fs_rename(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "rename", 4);
}


static void
wrap_pre_uv_fs_rmdir(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "rmdir", 3);
}


static void wrap_pre_uv_fs_stat(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "stat", 3);
}


static void wrap_pre_uv_fs_symlink(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "symlink", 5);
}


static void
wrap_pre_uv_fs_unlink(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "unlink", 3);
}


static void
wrap_pre_uv_fs_utime(void *wrapctx, OUT void **user_data)
{
  node_utils::AddSubmitOp(wrapctx, user_data, "utime", 5);
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
  Event *last_event = (Event *) trace_gen->PopFromStore(LAST_CREATED_EVENT);
  if (last_event && trace_gen->GetCurrentBlock()) {
    trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
        async_id, *last_event));
    int *event_id = new int(async_id);
    trace_gen->AddToStore(FUNC_ARGS + "wrap_pre_emit_init",
        (void *) event_id);
    delete last_event;
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
  Event event = Event(Event::W, 1);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(), event));
}


static void
wrap_pre_fsreq(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  Event *ev = new Event(Event::W, 2);
  trace_gen->AddToStore(LAST_CREATED_EVENT, ev);
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
  Event ev = Event(Event::W, 3);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(), ev));
}


namespace generator {

wrapper_t NodeTraceGenerator::GetWrappers() {
  wrapper_t wrappers;

  // system calls
  wrappers["access"]   = { wrap_pre_access, nullptr };
  wrappers["chmod"]    = { wrap_pre_chmod, nullptr };
  wrappers["chown"]    = { wrap_pre_chown, nullptr };
  wrappers["open"]     = { wrap_pre_open, nullptr };
  wrappers["close"]    = { wrap_pre_close, nullptr };
  wrappers["lchown"]   = { wrap_pre_lchown, nullptr };
  wrappers["link"]     = { wrap_pre_link, nullptr };
  wrappers["lstat"]    = { wrap_pre_lstat, nullptr };
  wrappers["mkdir"]    = { wrap_pre_mkdir, nullptr };
  wrappers["readlink"] = { wrap_pre_readlink, nullptr };
  wrappers["realpath"] = { wrap_pre_realpath, nullptr };
  wrappers["rename"]   = { wrap_pre_rename, nullptr };
  wrappers["rmdir"]    = { wrap_pre_rmdir, nullptr };
  wrappers["stat"]     = { wrap_pre_stat, nullptr };
  wrappers["symlink"]  = { wrap_pre_symlink, nullptr };
  wrappers["unlink"]   = { wrap_pre_unlink, nullptr };
  wrappers["utime"]    = { wrap_pre_utime, nullptr };
  wrappers["open"]     = { wrap_pre_open, wrap_post_open };
  wrappers["close"]    = { wrap_pre_close, nullptr };

  // libuv wrappers for libuv functions responsible for executing
  // FS operations.
  wrappers["uv__fs_work"] = { wrap_pre_uv_fs_work, nullptr };
  wrappers["uv__fs_done"] = { wrap_pre_uv_fs_done, nullptr };

  // libuv wrappers
  wrappers["uv_fs_copyfile"] = { wrap_pre_uv_fs_copyfile, nullptr };
  wrappers["uv_fs_access"]   = { wrap_pre_uv_fs_access, nullptr };
  wrappers["uv_fs_chmod"]    = { wrap_pre_uv_fs_chmod, nullptr };
  wrappers["uv_fs_chown"]    = { wrap_pre_uv_fs_chown, nullptr };
  wrappers["uv_fs_open"]     = { wrap_pre_uv_fs_open, nullptr };
  wrappers["uv_fs_close"]    = { wrap_pre_uv_fs_close, nullptr };
  wrappers["uv_fs_lchown"]   = { wrap_pre_uv_fs_lchown, nullptr };
  wrappers["uv_fs_link"]     = { wrap_pre_uv_fs_link, nullptr };
  wrappers["uv_fs_lstat"]    = { wrap_pre_uv_fs_lstat, nullptr };
  wrappers["uv_fs_mkdir"]    = { wrap_pre_uv_fs_mkdir, nullptr };
  wrappers["uv_fs_readlink"] = { wrap_pre_uv_fs_readlink, nullptr };
  wrappers["uv_fs_realpath"] = { wrap_pre_uv_fs_realpath, nullptr };
  wrappers["uv_fs_rename"]   = { wrap_pre_uv_fs_rename, nullptr };
  wrappers["uv_fs_rmdir"]    = { wrap_pre_uv_fs_rmdir, nullptr };
  wrappers["uv_fs_stat"]     = { wrap_pre_uv_fs_stat, nullptr };
  wrappers["uv_fs_symlink"]  = { wrap_pre_uv_fs_symlink, nullptr };
  wrappers["uv_fs_unlink"]   = { wrap_pre_uv_fs_unlink, nullptr };
  wrappers["uv_fs_utime"]    = { wrap_pre_uv_fs_utime, nullptr };

  // Node wrappers
  wrappers["node::Start"]                                   = { wrap_pre_start, nullptr };
  wrappers["node::Environment::AsyncHooks::push_async_ids"] = { wrap_pre_emit_before, nullptr };
  wrappers["node::AsyncWrap::EmitAfter"]                    = { wrap_pre_emit_after, nullptr };
  wrappers["node::AsyncWrap::EmitAsyncInit"]                = { wrap_pre_emit_init, nullptr };
  wrappers["node::(anonymous namespace)::TimerWrap::now"]   = { wrap_pre_timerwrap, nullptr };
  wrappers["node::fs::NewFSReqWrap"]                        = { wrap_pre_fsreq, nullptr };
  wrappers["node::AsyncWrap::EmitPromiseResolve"]           = { wrap_pre_promise_resolve, nullptr };
  wrappers["node::PromiseWrap::PromiseWrap"]                = { wrap_pre_promise_wrap, nullptr };
  wrappers["node::AsyncWrap::NewAsyncId"]                   = { wrap_pre_new_async_id, nullptr };
  return wrappers;
}

}
