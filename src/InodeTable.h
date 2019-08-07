#ifndef INODE_TABLE_H
#define INODE_TABLE_H

#include <experimental/filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <utility>
#include <set>

#include "Table.h"


#define ROOT_INODE 0


using namespace std;
namespace fs = experimental::filesystem;


namespace table {

typedef size_t inode_t;
typedef pair<inode_t, string> inode_key_t;
typedef map<pair<inode_t, string>, inode_t> inode_table_t;


class InodeTable : public Table<inode_key_t, inode_t> {
  public:
    InodeTable():
      next_inode(1) {
        InodeTable::AddEntry(ROOT_INODE, "/", "/");
    };

    void AddEntry(inode_t inode_p, string basename, string p);
    void AddEntry(inode_t inode_p, string basename, string p, inode_t inode);
    void RemoveEntry(inode_t inode, string basename);
    optional<inode_t> GetInode(inode_t inode_p, string basename);
    void OpenInode(inode_t inode_p, string basename);
    void CloseInode(inode_t inode_p, string basename);
    inode_t ToInode(fs::path path_val);
    optional<fs::path> ToPath(inode_t inode);
    set<fs::path> ToPaths(inode_t inode);

  private:
    enum INodeType {
      LINKED,
      UNLINKED
    };

    map<inode_t, set<fs::path>> rev_table;
    Table<inode_key_t, pair<enum INodeType, size_t>> open_inodes;

    size_t next_inode;

    void RemoveInode(inode_t inode, string basename);
};


}


#endif
