#include "InodeTable.h"


namespace table {

void InodeTable::AddEntry(inode_t inode_p, string basename,
                          inode_t inode) {
  pair<inode_t, string> entry = { inode_p, basename };
  if (inode == ROOT_INODE) {
    table[entry] = next_inode;
    next_inode++;
  } else {
    table[entry] = inode;
  }
}


optional<inode_t> InodeTable::GetInode(inode_t inode_p, string basename) {
  return GetValue({ inode_p, basename });
}


inode_t InodeTable::ToInode(string filename) {
  // TODO
  return 0;
}


optional<string> InodeTable::ToPath(inode_t inode) {
  // TODO;
  optional<string> filename;
  return filename;
}


}
