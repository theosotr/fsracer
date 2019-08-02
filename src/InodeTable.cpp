#include "InodeTable.h"


namespace {

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


void InodeTable::RemoveEntry(inode_t inode_p, string basename) {
  inode_table_t::iterator it = table.find({ inode_p, basename });
  if (it != table.end()) {
    table.erase(it);
  }
}


optional<inode_t> InodeTable::GetInode(inode_t inode_p, string basename) {
  optional<inode_t> inode;
  inode_table_t::iterator it = table.find({ inode_p, basename });
  if (it != table.end()) {
    inode = it->second;
  }
  return inode;
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
