#include <assert.h>
#include <set>

#include "InodeTable.h"


namespace table {


void InodeTable::AddEntry(inode_t inode_p, string basename, string p) {
  AddEntry(inode_p, basename, p, next_inode);
  next_inode++;
}


void InodeTable::AddEntry(inode_t inode_p, string basename,
                          string p, inode_t inode) {
  // Add the given entry to the underlying inode table
  // and the reversed inode table.
  set<fs::path> path_set;
  pair<inode_t, string> entry = { inode_p, basename };
  path_set = ToPaths(inode);
  path_set.insert(p);
  rev_table[inode] = path_set;
  table[entry] = inode;
}


void InodeTable::RemoveEntry(inode_t inode_p, const string &basename) {
  optional<fs::path> path_p = ToPath(inode_p);
  if (!path_p.has_value()) {
    return;
  }

  // First, create the absolute path that corresponds to
  // the given parent inode and basename.
  fs::path absolute_path = path_p.value() / basename;
  inode_key_t key = { inode_p, basename };
  // We check whether the inode is open by any handler.
  auto val = open_inodes.GetValue(key);
  if (!val.has_value()) {
    // The inode is not open, so we just remove it from the corresponding
    // inode table.
    optional<inode_t> inode_opt = Table<inode_key_t, inode_t>::PopEntry(key);
    assert(inode_opt.has_value());
    inode_t inode = inode_opt.value();
    map<inode_t, set<fs::path>>::iterator it = rev_table.find(inode);
    if (it != rev_table.end()) {
      // Now remove the entry from the reversed inode table.
      // In other words, the inode will not be pointed to by
      // the absolute path corresponding to the given parent inode
      // and basename.
      it->second.erase(absolute_path);
      if (it->second.empty()) {
        // The resulting set is empty, so the inode is no longer
        // pointed to by any path.
        rev_table.erase(it);
      }
    }
  } else {
    switch (val.value().first) {
      // The inode is open. We mark it as UNLINKED so that it can be removed
      // whenever we close the corresponding handler.
      case LINKED:
        open_inodes.AddEntry(key, { UNLINKED, val.value().second });
      default:
        break;
    }
  }
}


optional<inode_t> InodeTable::GetInode(inode_t inode_p,
                                       const string &basename) const {
  return GetValue({ inode_p, basename });
}


void InodeTable::OpenInode(inode_t inode_p, const string &basename) {
  inode_key_t key = { inode_p, basename };
  auto val = open_inodes.GetValue(key);
  if (!val.has_value()) {
    // This inode was not currently open, but now it is.
    open_inodes.AddEntry(key, { LINKED, 1 });
  } else {
    // Increment the counter of open handlers that correspond to
    // the given inode.
    open_inodes.AddEntry(key, { LINKED, val.value().second + 1 });
  }
}


void InodeTable::CloseInode(inode_t inode_p, const string &basename) {
  inode_key_t key = { inode_p, basename };
  auto val = open_inodes.GetValue(key);
  if (!val.has_value()) {
    return;
  }
  enum INodeType inode_type = val.value().first;
  size_t counter = val.value().second;
  switch (inode_type) {
    case LINKED:
      switch (counter) {
        case 0:
          break;
        case 1:
          // The inode is LINKED and it's open only by the current handler.
          // So we remove it from the corresponding table.
          open_inodes.RemoveEntry(key);
          break;
        default:
          // The current inode is open by multiple handlers. We just
          // decrement the corresponding counter.
          open_inodes.AddEntry(key, { LINKED, counter - 1 });
      }
      break;
    case UNLINKED:
      switch (counter) {
        case 0:
          break;
        case 1:
          // The inode is unlinked and it is open only by one handler.
          // So it's time to remove the corresponding entry from the inode
          // table.
          open_inodes.RemoveEntry(key);
          RemoveEntry(inode_p, basename);
          break;
        default:
          // The inode is open by multiple handlers.
          // We just decrement the counter.
          open_inodes.AddEntry(key, { UNLINKED, counter - 1 });

    }
  }
}


inode_t InodeTable::ToInode(const fs::path &path_val) {
  if (path_val.native() == "/") {
    return ROOT_INODE + 1;
  }

  fs::path dirname = path_val.parent_path();
  fs::path basename = path_val.filename();
  inode_t inode_p = ToInode(dirname);
  optional<inode_t> i = GetInode(inode_p, basename.native());
  if (i.has_value()) {
    // The entry has been found in the inode table,
    // so return the retrieved inode.
    return i.value();
  } else {
    // Add a new entry to the inode table,
    // and return the generated inode as value.
    AddEntry(inode_p, basename.native(), path_val.native());
    return next_inode - 1;
  }
}


optional<fs::path> InodeTable::ToPath(inode_t inode) const {
  optional<fs::path> filename;
  set<fs::path> paths = ToPaths(inode);
  if (paths.size() == 1) {
    return *paths.begin();
  }

  // Since path.size() != 1, there are two cases:
  //   * There is not any path that points to the given inode.
  //   * There are multiple paths that point to the given inode.
  //
  // This method is supposed to be used to retrieve the path
  // that points to an inode corresponding to a directory.
  // In UNIX systems, it is guaranteed that an dir inode is
  // pointed by a single path.
  return filename;
}


set<fs::path> InodeTable::ToPaths(inode_t inode) const {
  set<fs::path> paths;
  map<inode_t, set<fs::path>>::const_iterator it = rev_table.find(inode);
  if (it != rev_table.end()) {
    return it->second;
  }
  return paths;
}


}
