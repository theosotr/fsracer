#include <optional>
#include <ostream>
#include <string>

#include "FSAnalyzer.h"
#include "Operation.h"
#include "Trace.h"
#include "Utils.h"

// TODO:
// * Generate erroneous messages when something goes wrong.

namespace analyzer {

void FSAnalyzer::Analyze(const TraceNode *trace_node) {
  if (trace_node) {
    analysis_time.Start();
    trace_node->Accept(this);
    analysis_time.Stop();
  }
}


void FSAnalyzer::AnalyzeTrace(const Trace *trace) {
  if (!trace) {
    return;
  }

  main_process = trace->GetThreadId();
  cwd = trace->GetCwd();

  cwd_table.AddEntry(main_process, inode_table.ToInode(cwd));

  vector<const ExecOp*> exec_ops = trace->GetExecOps();
  for (auto const &exec_op : exec_ops) {
    AnalyzeExecOp(exec_op);
  }

  vector<const Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
  
}


void FSAnalyzer::AnalyzeExecOp(const ExecOp *exec_op) {
  if (!exec_op) {
    return;
  }
  op_table.AddEntry(exec_op->GetId(), exec_op);
}


void FSAnalyzer::AnalyzeBlock(const Block *block) {
  if (!block) {
    return;
  }
  current_block = block;
  vector<const Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
}


void FSAnalyzer::AnalyzeExpr(const Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void FSAnalyzer::AnalyzeNewEvent(const NewEventExpr *new_ev_expr) {
  if (!new_ev_expr) {
    return;
  }
  event_info.AddEntry(new_ev_expr->GetEventId(), new_ev_expr);
}


void FSAnalyzer::AnalyzeSubmitOp(const SubmitOp *submit_op) {
  if (!submit_op) {
    return;
  }

  string op_id = submit_op->GetOpId();
  optional<const ExecOp*> exec_op = op_table.GetValue(op_id);
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
  vector<const Operation*> ops = exec_op.value()->GetOperations();
  for (auto const &op : ops) {
    AnalyzeOperation(op);
  }


}


void FSAnalyzer::AnalyzeOperation(const Operation *operation) {
  if (operation) {
    operation->Accept(this);
  }
}


void FSAnalyzer::AnalyzeNewFd(const NewFd *new_fd) {
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
  // the absolute path. To do so, we first get the inode of the parent path
  // and the basename of the initial path.
  //
  // The we add the entry to the inode table.
  inode_t inode_p = inode_table.ToInode(abs_path.value().parent_path());
  string basename = abs_path.value().filename().native();
  inode_table.AddEntry(inode_p, basename, abs_path.value());
  // Mark the inode as open.
  inode_table.OpenInode(inode_p, basename);
  fd_table.AddEntry({ main_process, new_fd->GetFd() }, { inode_p, basename });
}


void FSAnalyzer::AnalyzeDelFd(const DelFd *del_fd) {
  if (!del_fd) {
    return;
  }
  optional<inode_key_t> key = fd_table.PopEntry(
      { main_process, del_fd->GetFd() });
  if (key.has_value()) {
    // It's time to close the inode.
    inode_table.CloseInode(key.value().first, key.value().second);
  }
}


void FSAnalyzer::AnalyzeHpath(const Hpath *hpath) {
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
  ProcessPathEffect(p, hpath->GetEffectType(),
                    hpath->GetActualOpName());
}


void FSAnalyzer::AnalyzeHpathSym(const HpathSym *hpathsym) {
  if (!hpathsym) {
    return;
  }
  optional<fs::path> abs_path = GetAbsolutePath(
      hpathsym->GetDirFd(), hpathsym->GetPath());
  if (!abs_path.has_value()) {
    return;
  }
  // In the `hpathsym` construct, we do not dereference the link.
  ProcessPathEffect(abs_path.value(),
                    hpathsym->GetEffectType(),
                    hpathsym->GetActualOpName());
}


void FSAnalyzer::AnalyzeLink(const Link *link) {
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


void FSAnalyzer::AnalyzeRename(const Rename *rename) {
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
  UnlinkResource(inode_p, basename);
}


void FSAnalyzer::AnalyzeSymlink(const Symlink *symlink) {
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
    enum Hpath::EffectType effect,
    string operation_name) {
  DebugInfo debug_info;
  if (block_id == MAIN_BLOCK) {
    debug_info.AddDebugInfo("main");
  } else {
    auto expr = event_info.GetValue(block_id);
    debug_info = expr.value()->GetDebugInfo();
  }
  switch (effect) {
    case Hpath::CONSUMED:
    case Hpath::PRODUCED:
      effect_table.AddPathEffect(
          p, FSAccess(block_id, effect, debug_info, operation_name));
      break;
    case Hpath::EXPUNGED:
      inode_t inode_p = inode_table.ToInode(p.parent_path());
      string basename = p.filename().native();
      effect_table.AddPathEffect(
          p, FSAccess(block_id, Hpath::EXPUNGED, debug_info, operation_name));
      UnlinkResource(inode_p, basename);
  }
}


optional<fs::path> FSAnalyzer::GetParentDir(size_t dirfd) const {
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
      optional<inode_key_t> key = fd_table.GetValue({ main_process, dirfd });
      if (key.has_value()) {
        inode = inode_table.GetInode(key.value().first,
                                     key.value().second);
      }

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
  return p;
}


optional<fs::path> FSAnalyzer::GetAbsolutePath(size_t dirfd, fs::path p) const {
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


void FSAnalyzer::UnlinkResource(inode_t inode_p, string basename) {
  inode_table.RemoveEntry(inode_p, basename);
}


void FSAnalyzer::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  switch (out_format) {
    case JSON:
      DumpJSON(out->OutStream());
      break;
    case CSV:
      DumpCSV(out->OutStream());
      break;
  }
  delete out;
}


void FSAnalyzer::DumpJSON(ostream &os) const {
  os << "{" << endl;
  for (auto map_it = effect_table.begin(); map_it != effect_table.end(); map_it++) {
    auto entry = *map_it;
    os << "  \"" << entry.first.native() << "\": [" << endl;
    for (auto it = entry.second.begin(); it != entry.second.end(); it++) {
      os << "    {" << endl;
      os << "      \"block\": " << "\"" << (*it).event_id
        << "\"," << endl;
      os << "      \"effect\": " << "\""
        << Hpath::EffToString((*it).effect_type) << "\"" << endl;
      if (it != entry.second.end() - 1) {
        os << "    }," << endl;
      } else {
        os << "    }" << endl;
      }
    }
    if (map_it != --effect_table.end()) {
      os << "  ]," << endl;
    } else {
      os << "  ]" << endl;
    }
  }
  os << "}" << endl;
}


void FSAnalyzer::DumpCSV(ostream &os) const {
  for (auto const &entry : effect_table) {
    for (auto const &fs_access : entry.second) {
      os << entry.first.native() << ","
        << fs_access.event_id << ","
        << Hpath::EffToString(fs_access.effect_type) << "\n";
    }
  }
}


}
