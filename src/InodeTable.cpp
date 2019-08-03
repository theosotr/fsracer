#include<set>

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


void InodeTable::RemoveEntry(inode_t inode_p, string basename) {
  optional<fs::path> path_p = ToPath(inode_p);
  if (!path_p.has_value()) {
    return;
  }

  // First, create the absolute path that corresponds to
  // the given parent inode and basename.
  fs::path absolute_path = path_p.value() / basename;
  // Remove entry from the inode table.
  inode_t inode = Table<pair<inode_t, string>, inode_t>::PopEntry(
      { inode_p, basename });
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
}


optional<inode_t> InodeTable::GetInode(inode_t inode_p, string basename) {
  return GetValue({ inode_p, basename });
}


inode_t InodeTable::ToInode(fs::path path_val) {
  if (path_val.native() == "/") {
    return ROOT_INODE + 1;
  }

  fs::path dirname = path_val.parent_path();
  fs::path basename = path_val.filename();
  cout << "Dirname " << dirname << "\n";
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


optional<fs::path> InodeTable::ToPath(inode_t inode) {
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


set<fs::path> InodeTable::ToPaths(inode_t inode) {
  set<fs::path> paths;
  map<inode_t, set<fs::path>>::iterator it = rev_table.find(inode);
  if (it != rev_table.end()) {
    return it->second;
  }
  return paths;
}


}
