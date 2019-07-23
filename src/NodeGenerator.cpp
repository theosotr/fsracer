#include <set>
#include <stack>

#include "Generator.h"
#include "NodeGenerator.h"
#include "Utils.h"


#define NEW_TIMERWRAP "node::(anonymous namespace)::TimerWrap::New"
#define WORKER_OFFSET 336
#define PRE_WRAP(FUNC) pre_wrap<decltype(&FUNC), &FUNC>



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


static void
add_to_set(Generator *trace_gen, int event_id, string key)
{
  void *value = trace_gen->GetStoreValue(key);
  if (!value) {
    return;
  }

  set<int> *set_ptr = static_cast<set<int>*>(value);
  set_ptr->insert(event_id);
}


static bool
has_event_id(Generator *trace_gen, int event_id, string key)
{

  void *value = trace_gen->GetStoreValue(key);
  if (!value) {
    return false;
  }

  set<int> *set_ptr = static_cast<set<int>*>(value);
  set<int>::iterator it = set_ptr->find(event_id);
  return !(it == set_ptr->end());
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
  exec_op->AddOperation(new NewFd(AT_FDCWD, path, ret_val));
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
wrap_pre_uv_fs_work(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
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
  char *ptr = (char *) drwrap_get_arg(wrapctx, 0);
  void *uv_fs_t_ptr = ptr - WORKER_OFFSET;

  // We convert address to a string and we use it
  // as key to store value information about the execution of
  // the current libuv operation.
  //
  // Note that the address is unique for every *active* libuv
  // operation.
  string addr = utils::PtrToString(uv_fs_t_ptr);

  string thread_str = to_string(utils::GetCurrentThread(wrapctx));
  string key = FUNC_INVOCATIONS + thread_str + "/uv__fs_work";
  // We get the number of `uv__fs_work` invocations in the current call
  // stack.
  int *work_count = (int *) trace_gen->GetStoreValue(key);
  if (!work_count) {
    // It's the first `uv__fs_work` invocation in the current call stack
    // so we update the store.
    work_count = new int(1);
    trace_gen->AddToStore(key, (void *) work_count);
  } else {
    // We increment the number of invocations of `uv__fs_work` in the current
    // call stack.
    (*work_count)++;
  }

  // Now it's time to retrieve the pointer to the `ExecOp` objects.
  // This object holds all the FStrace operations performed by
  // the current `libuv` operation.
  //
  // Note tha we consider only the parent `uv__fs_work` invocation
  // that is actually called by Node.
  key = OPERATIONS + addr;
  void *value = trace_gen->PopFromStore(key);
  if (!value) {
    // Probably, this invocation is not the parent,
    // so we return.
    return;
  }
  string *op_ptr = static_cast<string*>(value);
  // We create a new `ExecOp`, since its the first `uv__fs_work`
  // invocation in the current call stack.
  ExecOp *exec_op = new ExecOp(*op_ptr);
  delete op_ptr;
  
  // We associate the current thread with the newly-created
  // `ExecOp` object.
  trace_gen->AddToStore(THREADS + thread_str, (void *) exec_op);
  // We add the `ExecOp` construct to the current trace.
  trace_gen->GetTrace()->AddExecOp(exec_op);
}


static void
wrap_post_uv_fs_work(void *wrapctx, void *user_data)
{

  Generator *trace_gen = (Generator *) user_data;
  size_t thread_id = utils::GetCurrentThread(wrapctx);
  string thread_str = to_string(thread_id);

  // We examine the store to see the number of
  // 'uv__fs_work' invocations in the current call stack.
  string key = FUNC_INVOCATIONS + thread_str + "/uv__fs_work";
  int *work_count = (int *) trace_gen->GetStoreValue(key);
  if (!work_count) {
    return;
  }

  if (*work_count == 0 || *work_count == 1) {
    // This means that only only one invocation of `uv__fs_work`
    // appears in the stack. So after the call of this wrapper,
    // there will be no invocation of `uv__fs_work` in the call stack.
    //
    // So, we free memory and delete the corresponding keys from the store.
    delete work_count;
    trace_gen->DeleteFromStore(key);
    trace_gen->DeleteFromStore(THREADS + thread_str);
  }
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
    // The block we are going to create is a nested block.
    // 
    // Nested blocks are not allowed to our trace language.
    //
    // Therefore, we don't create a new block. Instead, we use
    // the most-recent allocated block.
    size_t block_id = trace_gen->GetCurrentBlock()->GetBlockId();
    if (block_id != MAIN_BLOCK) {
      trace_gen->GetCurrentBlock()->AddExpr(
          new LinkExpr(block_id, async_id));
      return;
    } 
  }

  int id = async_id;
  if (has_event_id(trace_gen, async_id, PROMISE_SET)) {
    // If `async_id` is a promise, we use that as the id
    // of the block.
    id = trigger_async_id;
  }


  if (trace_gen->GetCurrentBlock()) {
    // If this values does not point to nullptr, we can infer
    // that we did not close the previous block.
    //
    // So we do it right now.
    //trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
    trace_gen->SetCurrentBlock(nullptr);
  }
  // This hook occurs just before the execution of an event-related callback.
  trace_gen->SetCurrentBlock(new Block(id));
  trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
}


static void
wrap_pre_emit_after(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  int async_id = *(double *) ctx->ymm; // xmm0 register

  if (!trace_gen->GetCurrentBlock()) {
    // The current block is pointing to null; so there's nothing to end.
    return;
  }

  size_t block_id = trace_gen->GetCurrentBlock()->GetBlockId();
  if (block_id == async_id) {
    // The current block id matches with the id of the block
    // we are going to close.
    trace_gen->SetCurrentBlock(nullptr);
    return;
  }

  if (has_event_id(trace_gen, async_id, PROMISE_SET)) {
    // The id of the block we are going to close is promise-related.
    // We close it because we know that promise-related blocks are not
    // nested. XXX: revisit.
    trace_gen->SetCurrentBlock(nullptr);
  }
}


inline static void
add_new_event_expr(Generator *trace_gen, size_t event_id, Event event)
{
  if (!trace_gen) {
    return;
  }
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(event_id, event));
  int *event_id_ptr = new int(event_id);
  trace_gen->AddToStore(FUNC_ARGS + "wrap_pre_emit_init",
      (void *) event_id_ptr);
}


static void
wrap_pre_emit_init(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  int async_id = *(double *) ctx->ymm; // xmm0 register
  int trigger_async_id = *((double *) ctx->ymm + 8); // xmm1 register
  trace_gen->IncrEventCount();

  string top_func = trace_gen->TopStack();
  if (top_func == NEW_TIMERWRAP) {
    // This means that the created event is a timer wrapper.
    // So we create an event of type W 0.
    //
    // The execution order of the related callbacks follows the
    // order that appear in traces.
    add_new_event_expr(trace_gen, async_id, Event(Event::W, 0));
    return;
  }

  bool *is_promise = (bool *) trace_gen->PopFromStore(PROMISE_EVENT);
  if (is_promise && *is_promise) {
    // This event we are going to create is a promise,
    // so we add the corresponding id to the set of promises.
    add_to_set(trace_gen, async_id, PROMISE_SET);
  }

  if (is_promise) {
    // Allocated at `wrap_pre_promise_wrap()`.
    delete is_promise;
    // Since the current event resource is a promise,
    // we do not create a newEvent expression at this point.
    //
    // For promises, we consider the time when they are fulfilled
    // or rejected..
    return;
  }

  // The default event is of type W 2.
  Event last_event = Event(Event::W, 2);
  add_new_event_expr(trace_gen, async_id, last_event);
  // We link the event with `trigger_async_id` with the event
  // with id related to `async_id`.
  Block *current_block = trace_gen->GetCurrentBlock();
  current_block->AddExpr(
      new LinkExpr(trigger_async_id, async_id));
}


static void
wrap_pre_start(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  // This is the block corresponding to the execution of the top-level
  // code.
  //
  // We set the ID of this block to 1.
  set<int> initial_set;
  set<int> *promises = new set<int>(initial_set);
  set<int> *timers = new set<int>(initial_set);
  trace_gen->AddToStore(PROMISE_SET, (void *) promises);
  trace_gen->AddToStore(TIMER_SET, (void *) timers);
  trace_gen->SetCurrentBlock(new Block(MAIN_BLOCK));
  trace_gen->GetTrace()->AddBlock(trace_gen->GetCurrentBlock());
  trace_gen->IncrEventCount();
}


static void
wrap_pre_new_tick_info(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  Event event = Event(Event::S, 0);
  // We remove the last expression created by the NewAsyncId function.
  trace_gen->GetCurrentBlock()->PopExpr();
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(), event));
}


static void
wrap_pre_timerwrap(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  Event event = Event(Event::W, 1);
  // We remove the last expression created by the NewAsyncId function.
  trace_gen->GetCurrentBlock()->PopExpr();
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      trace_gen->GetEventCount(), event));
  add_to_set(trace_gen, trace_gen->GetEventCount(), TIMER_SET);
}


static void
wrap_pre_promise_resolve(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  dr_mcontext_t *ctx = drwrap_get_mcontext(wrapctx);
  int async_id = *(double *) ctx->ymm; // xmm0 register
  trace_gen->IncrEventCount();
  Event event = Event(Event::S, 0);
  trace_gen->GetCurrentBlock()->AddExpr(new NewEventExpr(
      async_id, event));
}


static void
wrap_pre_thenable(void *wrapctx, OUT void **user_data)
{
  // The underlying V8 function NewPromiseResolveThenableJobTask()
  // is called when we asynchronously resolve a promise with a thenable
  // or another promise object.
  //
  // For example, consider the following JavaScript code:
  //
  // var t = {
  //   then: function foo(resolve) => {
  //       resolve(true);
  //   }
  // var p = Promise.resolve(t);
  //
  // The promise `p` does not resolve synchronously.
  // Instead, V8 schedules the execution of function foo()
  // that eventually resolves the promise `p`.
  //
  // Therefore, when we encounter an invocation of the internal
  // V8 function NewPromiseResolveThenableJobTask(), we remove
  // the last expression of the current execution block
  // corresponding to the creation of the promise.
  //
  // A similar expression is added *after* the execution of
  // the function responsible for fulfilling the promise (e.g., foo()).
  //
  Generator *trace_gen = GetTraceGenerator(user_data);
  trace_gen->GetCurrentBlock()->PopExpr();
}


static void
wrap_pre_promise_wrap(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  bool *is_promise = new bool(true);
  trace_gen->AddToStore(PROMISE_EVENT, (void *) is_promise);
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
  wrappers["uv__fs_work"] = { wrap_pre_uv_fs_work, wrap_post_uv_fs_work };

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
  wrappers["node::Start"]                                   = { PRE_WRAP(wrap_pre_start), DefaultPost };
  wrappers["node::Environment::AsyncHooks::push_async_ids"] = { PRE_WRAP(wrap_pre_emit_before), DefaultPost };
  wrappers["node::Environment::AsyncHooks::pop_async_id"]   = { PRE_WRAP(wrap_pre_emit_after), DefaultPost };
  wrappers["node::AsyncWrap::EmitAsyncInit"]                = { PRE_WRAP(wrap_pre_emit_init), DefaultPost };
  wrappers["node::(anonymous namespace)::TimerWrap::Now"]   = { PRE_WRAP(wrap_pre_timerwrap), DefaultPost };
  wrappers["node::(anonymous namespace)::TimerWrap::New"]   = { DefaultPre, DefaultPost };
  //wrappers["node::fs::NewFSReqWrap"]                        = { PRE_WRAP(wrap_pre_fsreq), DefaultPost };
  wrappers["node::AsyncWrap::EmitPromiseResolve"]           = { PRE_WRAP(wrap_pre_promise_resolve), DefaultPost };
  wrappers["node::PromiseWrap::New"]                        = { PRE_WRAP(wrap_pre_promise_wrap), DefaultPost };
  wrappers["node::AsyncWrap::NewAsyncId"]                   = { PRE_WRAP(wrap_pre_new_async_id), DefaultPost };
  wrappers["node::AsyncWrap::NewTickInfo"]                  = { PRE_WRAP(wrap_pre_new_tick_info), DefaultPost };

  // V8 wrappers
  wrappers["v8::internal::Factory::NewPromiseResolveThenableJobTask"] = { wrap_pre_thenable, nullptr };
  return wrappers;
}


void NodeTraceGenerator::Stop() {
  // We deallocate the set pointers holding all the timer-
  // and promise-related events.
  set<int> *set_ptr;
  void *value = this->PopFromStore(PROMISE_SET);
  if (value) {
    set_ptr = static_cast<set<int>*>(value);
    delete set_ptr;
  }

  value = this->PopFromStore(TIMER_SET);
  if (value) {
    set_ptr = static_cast<set<int>*>(value);
    delete set_ptr;
  }
}


}
