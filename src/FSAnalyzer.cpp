#include <optional>
#include <ostream>
#include <string>

#include "FSAnalyzer.h"
#include "Operation.h"
#include "Trace.h"
#include "Utils.h"

// TODO:
// * Properly unlink resources.
// * Generate erroneous messages when something goes wrong.

namespace analyzer {

void FSAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void FSAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }

  main_process = trace->GetThreadId();
  cwd = trace->GetCwd();

  cwd_table.AddEntry(main_process, inode_table.ToInode(cwd));

  vector<ExecOp*> exec_ops = trace->GetExecOps();
  for (auto const &exec_op : exec_ops) {
    AnalyzeExecOp(exec_op);
  }

  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
  
}


void FSAnalyzer::AnalyzeExecOp(ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  op_table.AddEntry(exec_op->GetId(), exec_op);
}


void FSAnalyzer::AnalyzeBlock(Block *block) {
  if (!block) {
    return;
  }
  current_block = block;
  vector<Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
}


void FSAnalyzer::AnalyzeExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void FSAnalyzer::AnalyzeSubmitOp(SubmitOp *submit_op) {
  if (!submit_op) {
    return;
  }

  string op_id = submit_op->GetId();
  optional<ExecOp*> exec_op = op_table.GetValue(op_id);
  if (!exec_op.has_value()) {
    return;
  }
  switch (submit_op->GetType()) {
    case SubmitOp::SYNC:
      block_id = current_block->GetBlockId();
      break;
    case SubmitOp::ASYNC:
      block_id = stoi(utils::GetRightSubstr(op_id, "_"));
      break;
  }
  vector<Operation*> ops = exec_op.value()->GetOperations();
  for (auto const &op : ops) {
    AnalyzeOperation(op);
  }


}


void FSAnalyzer::AnalyzeOperation(Operation *operation) {
  if (operation) {
    operation->Accept(this);
  }
}


void FSAnalyzer::AnalyzeNewFd(NewFd *new_fd) {
  if (!new_fd) {
    return;
  }
  if (new_fd->GetFd() < 0) {
    // The corresponding system call failed to create
    // a new file descriptor. So we do nothing.
    return;
  }
  optional<fs::path> abs_path = GetAbsolutePath(
      new_fd->GetDirFd(), new_fd->GetPath());
  if (!abs_path.has_value()) {
    return;
  }
  // We create a new inode that corresponds to
  // the absolute path.
  inode_t inode = inode_table.ToInode(abs_path.value());
  fd_table.AddEntry({ main_process, new_fd->GetFd() }, inode);
}


void FSAnalyzer::AnalyzeDelFd(DelFd *del_fd) {
  if (!del_fd) {
    return;
  }
  fd_table.RemoveEntry({ main_process, del_fd->GetFd() });
}


void FSAnalyzer::AnalyzeHpath(Hpath *hpath) {
  if (!hpath) {
    return;
  }
  optional<fs::path> abs_path = GetAbsolutePath(
      hpath->GetDirFd(), hpath->GetPath());
  if (!abs_path.has_value()) {
    return;
  }
  fs::path p = abs_path.value();
  inode_t inode = inode_table.ToInode(p);
  // At this point, we check whether this path corresponds
  // to a symbolic link.
  // If this is the case, we dereference the link.
  optional<fs::path> der_path = symlink_table.GetValue(inode);
  if (der_path.has_value()) {
    p = der_path.value();
  }
  ProcessPathEffect(p, hpath->GetEffectType());
}


void FSAnalyzer::AnalyzeHpathSym(HpathSym *hpathsym) {
  if (!hpathsym) {
    return;
  }
  optional<fs::path> abs_path = GetAbsolutePath(
      hpathsym->GetDirFd(), hpathsym->GetPath());
  if (!abs_path.has_value()) {
    return;
  }
  // In the `hpathsym` construct, we do not dereference the link.
  ProcessPathEffect(abs_path.value(), hpathsym->GetEffectType());
}


void FSAnalyzer::AnalyzeLink(Link *link) {
  if (!link) {
    return;
  }
  optional<fs::path> old_path = GetAbsolutePath(
      link->GetOldDirfd(), link->GetOldPath());
  optional<fs::path> new_path = GetAbsolutePath(
      link->GetNewDirfd(), link->GetNewPath());
  if (!old_path.has_value() || !new_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(old_path.value());
  inode_t inode_p = inode_table.ToInode(
      new_path.value().parent_path());
  string basename = new_path.value().filename().native();
  inode_table.AddEntry(inode_p, basename, new_path.value().native(),
                       inode);
}


void FSAnalyzer::AnalyzeRename(Rename *rename) {
  if (!rename) {
    return;
  }
  optional<fs::path> old_path = GetAbsolutePath(
      rename->GetOldDirfd(), rename->GetOldPath());
  optional<fs::path> new_path = GetAbsolutePath(
      rename->GetNewDirfd(), rename->GetNewPath());
  if (!old_path.has_value() || !new_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(old_path.value());
  inode_t inode_p = inode_table.ToInode(
      new_path.value().parent_path());
  string basename = new_path.value().filename().native();
  optional<inode_t> inode_new = inode_table.GetValue(
      { inode_p, basename });
  if (inode_new.has_value() && inode_new.value() == inode) {
    return;
  }
  inode_table.AddEntry(inode_p, basename, new_path.value().native(),
                       inode);
  inode_p = inode_table.ToInode(old_path.value().parent_path());
  basename = old_path.value().filename().native();
  inode_table.RemoveEntry(inode_p, basename);
}


void FSAnalyzer::AnalyzeSymlink(Symlink *symlink) {
  if (!symlink) {
    return;
  }
  optional<fs::path> abs_path = GetAbsolutePath(
      symlink->GetDirFd(), symlink->GetPath());
  if (!abs_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(abs_path.value());
  symlink_table.AddEntry(inode, symlink->GetTargetPath());
}


void FSAnalyzer::ProcessPathEffect(fs::path p,
    enum Hpath::EffectType effect) {
  switch (effect) {
    case Hpath::CONSUMED:
    case Hpath::PRODUCED:
      effect_table.AddPathEffect(p, block_id, effect);
      break;
    case Hpath::EXPUNGED:
      inode_t inode_p = inode_table.ToInode(p.parent_path());
      string basename = p.filename().native();
      inode_table.RemoveEntry(inode_p, basename);
      effect_table.AddPathEffect(p, block_id, Hpath::EXPUNGED);
  }
}


optional<fs::path> FSAnalyzer::GetParentDir(size_t dirfd) {
  optional<fs::path> cwd;
  optional<inode_t> inode;
  switch (dirfd) {
    case AT_FDCWD:
      // The dir file descriptor is the `AT_FDCWD` flag.
      // So we get the inode corresponding to the current
      // working directory of the process.
      inode = cwd_table.GetValue(main_process);
      break;
    default:
      // Otherwise, we inspect the file descriptor table to
      // get the inode corresponding to the given file descriptor.
      inode = fd_table.GetValue({ main_process, dirfd });

  }
  if (!inode.has_value()) {
    return cwd;
  }
  // Convert the inode to a path. Since we know that this
  // inode corresponds to a directory, it cannot be the case
  // where multiple paths point to the same inode.
  //
  // Unix system do not allow hard links in directories.
  optional<fs::path> p = inode_table.ToPath(inode.value());
  if (!p.has_value()) {
    return cwd;
  }
  cout << p.value() << endl;
  return p;
}


optional<fs::path> FSAnalyzer::GetAbsolutePath(size_t dirfd, fs::path p) {
  if (p.is_absolute()) {
    return p;
  }
  optional<fs::path> parent_p = GetParentDir(dirfd);
  if (!parent_p.has_value()) {
    return parent_p;
  }
  // TODO better normalise path.
  return parent_p.value() / p;
}


void FSAnalyzer::DumpOutput(writer::OutWriter *out) {
  if (!out) {
    return;
  }

  ostream &os = out->OutStream();
  os << "{" << endl;
  for (auto const &entry : effect_table.GetTable()) {
    os << "\"" << entry.first.native() << "\": [" << endl;
    for (auto const &pair_element : entry.second) {
      os << "{" << endl;
      os << "\"block\": " << "\"" << pair_element.first << "\"," << endl;
      os << "\"effect\": " << "\"" << Hpath::EffToString(pair_element.second)
        << "\"" << endl;
      os << "}" << endl;
    }
    os << "]" << endl;
  }
  os << "}" << endl;

  delete out;
  out = nullptr;
}


}
