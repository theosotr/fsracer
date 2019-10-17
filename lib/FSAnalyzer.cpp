#include "FSAnalyzer.h"


namespace analyzer {


std::string FSAnalyzer::GetName() const {
  return "FSAnalyzer";
}


void FSAnalyzer::AnalyzeExecTaskBeg(const fstrace::ExecTaskBeg *exec_task) {
  if (!exec_task) {
    return;
  }
  current_task = exec_task->GetTaskName();
}


void FSAnalyzer::AnalyzeSysOpBeg(const fstrace::SysOpBeg *sysop) {
  if (!sysop) {
    return;
  }
  in_sysop = true;
}


void FSAnalyzer::AnalyzeEnd(const fstrace::End *end) {
  if (!end) {
    return;
  }
  if (in_sysop) {
    in_sysop = false;
  } else {
    current_task.reset();
  }
}


void FSAnalyzer::AnalyzeNewFd(const fstrace::NewFd *new_fd) {
  if (!new_fd) {
    return;
  }
  if (new_fd->GetFd() < 0) {
    // The corresponding system call failed to create
    // a new file descriptor. So we do nothing.
    return;
  }
  std::optional<fs::path> abs_path = GetAbsolutePath(
      new_fd->GetDirFd(), new_fd->GetFilename(), new_fd->GetPid());
  if (!abs_path.has_value()) {
    return;
  }
  // We create a new inode that corresponds to
  // the absolute path. To do so, we first get the inode of the parent path
  // and the basename of the initial path.
  //
  // The we add the entry to the inode table.
  inode_t inode_p = inode_table.ToInode(abs_path.value().parent_path());
  std::string basename = abs_path.value().filename().native();
  inode_table.AddEntry(inode_p, basename, abs_path.value());
  // Mark the inode as open.
  inode_table.OpenInode(inode_p, basename);
  fd_table.AddEntry({ new_fd->GetPid(), new_fd->GetFd() },
                    { inode_p, basename });
}


void FSAnalyzer::AnalyzeDelFd(const fstrace::DelFd *del_fd) {
  if (!del_fd) {
    return;
  }
  std::optional<inode_key_t> key = fd_table.PopEntry(
      { del_fd->GetPid(), del_fd->GetFd() });
  if (key.has_value()) {
    // It's time to close the inode.
    inode_table.CloseInode(key.value().first, key.value().second);
  }
}


void FSAnalyzer::AnalyzeDupFd(const fstrace::DupFd *dup_fd) {
  if (!dup_fd) {
    return;
  }
  switch (dup_fd->GetOldFd()) {
    case 0:
    case 1:
    case 2:
      break;
    default:
      if (dup_fd->GetOldFd() == dup_fd->GetNewFd()) {
        return;
      }
      std::optional<inode_key_t> key = fd_table.GetValue(
          { dup_fd->GetPid(), dup_fd->GetOldFd() });
      if (key.has_value()) {
        fd_table.AddEntry({ dup_fd->GetPid(), dup_fd->GetNewFd() },
                          key.value());
      }
  }
}


void FSAnalyzer::AnalyzeHpath(const fstrace::Hpath *hpath) {
  if (!hpath) {
    return;
  }
  std::optional<fs::path> abs_path = GetAbsolutePath(
      hpath->GetDirFd(), hpath->GetFilename(), hpath->GetPid());
  if (!abs_path.has_value()) {
    return;
  }
  fs::path p = abs_path.value();
  inode_t inode = inode_table.ToInode(p);
  // At this point, we check whether this path corresponds
  // to a symbolic link.
  // If this is the case, we dereference the link.
  std::optional<fs::path> der_path = symlink_table.GetValue(inode);
  if (der_path.has_value()) {
    p = der_path.value();
  }
  ProcessPathEffect(p, hpath->GetAccessType(),
                    hpath->GetActualOpName());
}


void FSAnalyzer::AnalyzeHpathSym(const fstrace::HpathSym *hpathsym) {
  if (!hpathsym) {
    return;
  }
  std::optional<fs::path> abs_path = GetAbsolutePath(
      hpathsym->GetDirFd(), hpathsym->GetFilename(), hpathsym->GetPid());
  if (!abs_path.has_value()) {
    return;
  }
  // In the `hpathsym` construct, we do not dereference the link.
  ProcessPathEffect(abs_path.value(),
                    hpathsym->GetAccessType(),
                    hpathsym->GetActualOpName());
}


void FSAnalyzer::AnalyzeLink(const fstrace::Link *link) {
  if (!link) {
    return;
  }
  std::optional<fs::path> old_path = GetAbsolutePath(
      link->GetOldDirfd(), link->GetOldPath(), link->GetPid());
  std::optional<fs::path> new_path = GetAbsolutePath(
      link->GetNewDirfd(), link->GetNewPath(), link->GetPid());
  if (!old_path.has_value() || !new_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(old_path.value());
  inode_t inode_p = inode_table.ToInode(
      new_path.value().parent_path());
  std::string basename = new_path.value().filename().native();
  inode_table.AddEntry(inode_p, basename, new_path.value().native(),
                       inode);
}


void FSAnalyzer::AnalyzeNewProc(const fstrace::NewProc *new_proc) {
  if (!new_proc) {
    return;
  }
  for (auto const &elem : fd_table.GetTable()) {
    auto key = elem.first;
    if (key.first == new_proc->GetPid()) {
      fd_table.AddEntry({ new_proc->GetNewProcId(), key.second }, elem.second);
    }
  }
  std::optional<inode_t> cwd_inode = cwd_table.GetValue(new_proc->GetPid());
  if (cwd_inode.has_value()) {
    // TODO
    cwd_table.AddEntry(new_proc->GetNewProcId(), cwd_inode.value());
  }
}



void FSAnalyzer::AnalyzeRename(const fstrace::Rename *rename) {
  if (!rename) {
    return;
  }
  std::optional<fs::path> old_path = GetAbsolutePath(
      rename->GetOldDirfd(), rename->GetOldPath(), rename->GetPid());
  std::optional<fs::path> new_path = GetAbsolutePath(
      rename->GetNewDirfd(), rename->GetNewPath(), rename->GetPid());
  if (!old_path.has_value() || !new_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(old_path.value());
  inode_t inode_p = inode_table.ToInode(
      new_path.value().parent_path());
  std::string basename = new_path.value().filename().native();
  std::optional<inode_t> inode_new = inode_table.GetValue(
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


void FSAnalyzer::AnalyzeSetCwd(const fstrace::SetCwd *setcwd) {
  if (!setcwd) {
    return;
  }
  std::optional<fs::path> abs_path = GetAbsolutePath(
      AT_FDCWD, setcwd->GetCwd(), setcwd->GetPid());
  if (!abs_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(abs_path.value());
  cwd_table.AddEntry(setcwd->GetPid(), inode);
}


void FSAnalyzer::AnalyzeSetCwdFd(const fstrace::SetCwdFd *setcwd) {
  if (!setcwd) {
    return;
  }
  std::optional<inode_key_t> key = fd_table.GetValue(
      { setcwd->GetPid(), setcwd->GetFd() });
  if (!key.has_value()) {
    return;
  }
  std::optional<inode_t> inode = inode_table.GetValue(key.value());
  if (!inode.has_value()) {
    return;
  }
  cwd_table.AddEntry(setcwd->GetPid(), inode.value());
}


void FSAnalyzer::AnalyzeSymlink(const fstrace::Symlink *symlink) {
  if (!symlink) {
    return;
  }
  std::optional<fs::path> abs_path = GetAbsolutePath(
      symlink->GetDirFd(), symlink->GetFilename(), symlink->GetPid());
  if (!abs_path.has_value()) {
    return;
  }
  inode_t inode = inode_table.ToInode(abs_path.value());
  symlink_table.AddEntry(inode, symlink->GetTargetPath());
}


void FSAnalyzer::ProcessPathEffect(fs::path p,
    enum fstrace::Hpath::AccessType access,
    std::string operation_name) {
  fstrace::DebugInfo debug_info;
  std::string task_name = current_task.has_value() ?
    current_task.value() : "main";
  switch (access) {
    case fstrace::Hpath::CONSUMED:
    case fstrace::Hpath::PRODUCED:
      AddPathEffect(
          p, FSAccess(task_name, access, debug_info, operation_name));
      break;
    case fstrace::Hpath::EXPUNGED:
      inode_t inode_p = inode_table.ToInode(p.parent_path());
      std::string basename = p.filename().native();
      AddPathEffect(
          p, FSAccess(task_name, fstrace::Hpath::EXPUNGED, debug_info,
                      operation_name));
      UnlinkResource(inode_p, basename);
  }
}


std::optional<fs::path> FSAnalyzer::GetParentDir(size_t dirfd,
                                                    size_t pid) const {
  std::optional<fs::path> cwd;
  std::optional<inode_t> inode;
  switch (dirfd) {
    case AT_FDCWD:
      // The dir file descriptor is the `AT_FDCWD` flag.
      // So we get the inode corresponding to the current
      // working directory of the process.
      inode = cwd_table.GetValue(pid);
      break;
    default:
      // Otherwise, we inspect the file descriptor table to
      // get the inode corresponding to the given file descriptor.
      std::optional<inode_key_t> key = fd_table.GetValue({ pid, dirfd });
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
  std::optional<fs::path> p = inode_table.ToPath(inode.value());
  if (!p.has_value()) {
    return cwd;
  }
  return p;
}


std::optional<fs::path> FSAnalyzer::GetAbsolutePath(size_t dirfd,
                                                       fs::path p,
                                                       size_t pid) const {
  if (p.is_absolute()) {
    return p;
  }
  std::optional<fs::path> parent_p = GetParentDir(dirfd, pid);
  if (!parent_p.has_value()) {
    return parent_p;
  }
  // TODO better normalise path.
  return parent_p.value() / p;
}


void FSAnalyzer::UnlinkResource(inode_t inode_p, std::string basename) {
  inode_table.RemoveEntry(inode_p, basename);
}


void FSAnalyzer::AddPathEffect(const fs::path &p, FSAccess fs_access) {
  std::string event_id = fs_access.task_name;
  auto key_pair = make_pair(p, event_id);
  std::optional<FSAccess> fs_acc_opt = task_accesses.GetValue(key_pair);
  if (!fs_acc_opt.has_value()) {
    // It's the first time the path `p` is accessed by the event
    // whose id is `event_id`.
    task_accesses.AddEntry(key_pair, fs_access);
    return;
  }
  FSAccess old_fs_acc = fs_acc_opt.value();
  switch (fs_access.access_type) {
    case fstrace::Hpath::CONSUMED:
      switch (old_fs_acc.access_type) {
        case fstrace::Hpath::CONSUMED:
          // We consume a path that we have already consumed.
          // So we keep only the fresh access.
          task_accesses.AddEntry(key_pair, fs_access);
          break;
        default:
          // We consume a path that we have either produced or
          // expunged in the past. We keep the previous access. 
          task_accesses.AddEntry(key_pair, old_fs_acc);
      }
      break;
    case fstrace::Hpath::PRODUCED:
      task_accesses.AddEntry(key_pair, fs_access);
      break;
    case fstrace::Hpath::EXPUNGED:
      switch (old_fs_acc.access_type) {
        case fstrace::Hpath::CONSUMED:
        case fstrace::Hpath::EXPUNGED:
          // We expunge a path that we have either consumed or
          // expunged in the past. We keep the fresh access only.
          task_accesses.AddEntry(key_pair, fs_access);
          break;
        case fstrace::Hpath::PRODUCED:
          // We expunge a path that we have produced in the past.
          // Therefore, there is not any access associated with
          // this event and path.
          task_accesses.RemoveEntry(key_pair);
          break;
      }
  }
}


void FSAnalyzer::UpdateAccessTable() const {
  for (const auto &elem : task_accesses) {
    auto p = elem.first.first;
    auto fs_access = elem.second;

    std::optional<std::vector<FSAccess>> fs_accesses = effect_table.GetValue(p);
    std::vector<FSAccess> v_fs_acc;
    if (fs_accesses.has_value()) {
      v_fs_acc = fs_accesses.value();
      v_fs_acc.push_back(fs_access);
    } else {
      v_fs_acc.push_back(fs_access);
    }
    effect_table.AddEntry(p, v_fs_acc);
  }
}


void FSAnalyzer::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  UpdateAccessTable();
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
      os << "    {" << std::endl;
      os << "      \"block\": " << "\"" << (*it).task_name
        << "\"," << endl;
      os << "      \"effect\": " << "\""
        << fstrace::Hpath::AccToString((*it).access_type) << "\"" << std::endl;
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
        << fs_access.task_name << ","
        << fstrace::Hpath::AccToString(fs_access.access_type) << "\n";
    }
  }
}


FSAnalyzer::fs_accesses_table_t FSAnalyzer::GetFSAccesses() const {
  UpdateAccessTable();
  return effect_table;
}


} // namespace analyzer
