#include "generator.h"
#include "node_generator.h"

using namespace generator;
using namespace generator_utils;
using namespace generator_keys;


namespace node_utils {

void AddOperation(Generator *trace_gen, Operation *operation, bool is_async) {
  if (!is_async) {
    trace_gen->GetCurrentBlock()->AddExpr(new SyncOp(operation));
  } else {
    size_t event_id = (size_t)(intptr_t) trace_gen->GetStoreValue(
        FUNC_ARGS + "wrap_pre_emit_init");
    trace_gen->GetCurrentBlock()->AddExpr(
        new AsyncOp(operation, event_id));
  }
}


void EmitHpath(void *wrapctx, OUT void **user_data, size_t path_pos,
               size_t clb_pos, enum Hpath::EffectType effect_type,
               bool follow_symlink) {
  Generator *trace_gen = GetTraceGenerator(user_data);
  string path = (const char *) drwrap_get_arg(wrapctx, path_pos);
  void *clb = drwrap_get_arg(wrapctx, clb_pos);
  if (follow_symlink) {
    Hpath *hpath = new Hpath(AT_FDCWD, path, effect_type);
    AddOperation(trace_gen, hpath, clb != nullptr);
  } else {
    HpathSym *hpathsym = new HpathSym(AT_FDCWD, path, effect_type);
    AddOperation(trace_gen, hpathsym, clb != nullptr);
  }
}


void EmitLink(void *wrapctx, OUT void **user_data, size_t old_path_pos,
    size_t new_path_pos, size_t clb_pos) {
  EmitHpath(wrapctx, user_data, old_path_pos, clb_pos, Hpath::CONSUMED, true);
  EmitHpath(wrapctx, user_data, new_path_pos, clb_pos, Hpath::PRODUCED, true);
  Generator *trace_gen = GetTraceGenerator(user_data);
  string old_path = (const char *) drwrap_get_arg(wrapctx, old_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  Link *link = new Link(AT_FDCWD, old_path, AT_FDCWD, new_path);
  AddOperation(trace_gen, link, drwrap_get_arg(wrapctx, clb_pos) != nullptr);
}


void EmitRename(void *wrapctx, OUT void **user_data, size_t old_path_pos,
    size_t new_path_pos, size_t clb_pos) {
  EmitHpath(wrapctx, user_data, old_path_pos, clb_pos, Hpath::EXPUNGED, true);
  EmitHpath(wrapctx, user_data, new_path_pos, clb_pos, Hpath::PRODUCED, true);
  Generator *trace_gen = GetTraceGenerator(user_data);
  string old_path = (const char *) drwrap_get_arg(wrapctx, old_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  Rename *rename = new Rename(AT_FDCWD, old_path, AT_FDCWD, new_path);
  AddOperation(trace_gen, rename, drwrap_get_arg(wrapctx, clb_pos) != nullptr);
}


void EmitSymlink(void *wrapctx, OUT void **user_data, size_t target_path_pos,
    size_t new_path_pos, size_t clb_pos) {
  EmitHpath(wrapctx, user_data, new_path_pos, clb_pos, Hpath::PRODUCED, true);
  Generator *trace_gen = GetTraceGenerator(user_data);
  string target_path = (const char *) drwrap_get_arg(wrapctx, target_path_pos);
  string new_path = (const char *) drwrap_get_arg(wrapctx, new_path_pos);
  Symlink *symlink = new Symlink(AT_FDCWD, new_path, target_path);
  AddOperation(trace_gen, symlink, drwrap_get_arg(wrapctx, clb_pos) != nullptr);
}

}


static void
wrap_pre_open(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }
  char *path = (char *) drwrap_get_arg(wrapctx, 0);
  string key = FUNC_ARGS + "open";
  trace_gen->AddToStore(key, path);
}


static void
wrap_post_open(void *wrapctx, void *user_data)
{
  Generator *trace_gen = (Generator *) user_data;
  void *drcontext = drwrap_get_drcontext(wrapctx);
  if (dr_get_thread_id(drcontext) == trace_gen->GetTrace()->GetThreadId()) {
    return;
  }

  string path = (const char *) trace_gen->PopFromStore(FUNC_ARGS + "open");
  string key = INCOMPLETE_OPERATIONS + NewFd(path).ToString();
  // We search whether there is an incomplete operation named `newFd`
  // that affects the same path as `open`.
  Operation *operation = (Operation *) trace_gen->PopFromStore(key);
  if (!operation) {
    // We did not find any operation; so we return.
    return;
  }

  // We set the `fd` value of the found `newFd` operation.
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  NewFd *new_fd = dynamic_cast<NewFd*>(operation);
  new_fd->SetFd(ret_val);
}


static void
wrap_pre_uv_fs_access(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 4, Hpath::CONSUMED, true);
}


static void
wrap_pre_uv_fs_chmod(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 4, Hpath::CONSUMED, true);
}


static void
wrap_pre_uv_fs_chown(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 5, Hpath::CONSUMED, true);
}


static void
wrap_pre_uv_fs_open(void *wrapctx, OUT void **user_data)
{
  Generator *trace_gen = GetTraceGenerator(user_data);
  string path = (const char *) drwrap_get_arg(wrapctx, 2);
  void *clb = drwrap_get_arg(wrapctx, 5);

  NewFd *new_fd = new NewFd(path);
  trace_gen->AddToStore(FUNC_ARGS + "uv_fs_open", new_fd);
}


static void
wrap_pre_uv_fs_post_open(void *wrapctx, void *user_data)
{
  Generator *trace_gen = (Generator *) user_data;
  int ret_val = (int)(ptr_int_t) drwrap_get_retval(wrapctx);
  string key = FUNC_ARGS + "uv_fs_open";
  NewFd *new_fd = (NewFd *) trace_gen->PopFromStore(key);
  SyncOp *op;
  if (ret_val == 0) {
    // If the return value of open is equal to zero,
    // we have an asynchronous open;
    string key = INCOMPLETE_OPERATIONS + new_fd->ToString(); 
    trace_gen->AddToStore(key, new_fd);
    size_t event_id = (size_t)(intptr_t) trace_gen->GetStoreValue(
        FUNC_ARGS + "wrap_pre_emit_init");
    op = new AsyncOp(new_fd, event_id);
  } else {
    new_fd->SetFd(ret_val);
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
  DelFd *del_fd = new DelFd(fd);
  node_utils::AddOperation(trace_gen, del_fd, clb != nullptr);
}


static void
wrap_pre_uv_fs_lchown(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 5, Hpath::CONSUMED, false);
}


static void
wrap_pre_uv_fs_link(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitLink(wrapctx, user_data, 2, 3, 4);
}


static void
wrap_pre_uv_fs_lstat(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::CONSUMED, false);
}


static void
wrap_pre_uv_fs_mkdir(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::PRODUCED, false);
}


static void
wrap_pre_uv_fs_readlink(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::CONSUMED, true);
}


static void
wrap_pre_uv_fs_realpath(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::CONSUMED, true);
}


static void
wrap_pre_uv_fs_rename(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitLink(wrapctx, user_data, 2, 3, 4);
}


static void
wrap_pre_uv_fs_rmdir(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::EXPUNGED, false);
}


static void wrap_pre_uv_fs_stat(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::CONSUMED, true);
}


static void wrap_pre_uv_fs_symlink(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitSymlink(wrapctx, user_data, 2, 3, 5);
}


static void
wrap_pre_uv_fs_unlink(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 3, Hpath::EXPUNGED, false);
}


static void
wrap_pre_uv_fs_utime(void *wrapctx, OUT void **user_data)
{
  node_utils::EmitHpath(wrapctx, user_data, 2, 5, Hpath::CONSUMED, true);
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
    trace_gen->AddToStore(FUNC_ARGS + "wrap_pre_emit_init",
        (void *)(intptr_t) async_id);
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
  wrappers["open"] = { wrap_pre_open, wrap_post_open };

  // libuv wrappers
  wrappers["uv_fs_access"] = { wrap_pre_uv_fs_access, nullptr };
  wrappers["uv_fs_chmod"] = { wrap_pre_uv_fs_chmod, nullptr };
  wrappers["uv_fs_chown"] = { wrap_pre_uv_fs_chown, nullptr };
  wrappers["uv_fs_open"] = { wrap_pre_uv_fs_open, wrap_pre_uv_fs_post_open };
  wrappers["uv_fs_close"] = { wrap_pre_uv_fs_close, nullptr };
  wrappers["uv_fs_lchown"] = { wrap_pre_uv_fs_lchown, nullptr };
  wrappers["uv_fs_link"] = { wrap_pre_uv_fs_link, nullptr };
  wrappers["uv_fs_lstat"] = { wrap_pre_uv_fs_lstat, nullptr };
  wrappers["uv_fs_mkdir"] = { wrap_pre_uv_fs_mkdir, nullptr };
  wrappers["uv_fs_readlink"] = { wrap_pre_uv_fs_readlink, nullptr };
  wrappers["uv_fs_realpath"] = { wrap_pre_uv_fs_realpath, nullptr };
  wrappers["uv_fs_rename"] = { wrap_pre_uv_fs_rename, nullptr };
  wrappers["uv_fs_rmdir"] = { wrap_pre_uv_fs_rmdir, nullptr };
  wrappers["uv_fs_stat"] = { wrap_pre_uv_fs_stat, nullptr };
  wrappers["uv_fs_symlink"] = { wrap_pre_uv_fs_symlink, nullptr };
  wrappers["uv_fs_unlink"] = { wrap_pre_uv_fs_unlink, nullptr };
  wrappers["uv_fs_utime"] = { wrap_pre_uv_fs_utime, nullptr };

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
