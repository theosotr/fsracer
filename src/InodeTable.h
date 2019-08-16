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

using inode_t = size_t;
using inode_key_t = pair<inode_t, string>;
using inode_table_t = map<pair<inode_t, string>, inode_t>;


class InodeTable : public Table<inode_key_t, inode_t> {
  public:
    InodeTable():
      next_inode(1) {
        InodeTable::AddEntry(ROOT_INODE, "/", "/");
    };

    void AddEntry(inode_t inode_p, string basename, string p);
    void AddEntry(inode_t inode_p, string basename, string p, inode_t inode);
    void RemoveEntry(inode_t inode, const string &basename);
    optional<inode_t> GetInode(inode_t inode_p, const string &basename) const;
    void OpenInode(inode_t inode_p, const string &basename);
    void CloseInode(inode_t inode_p, const string &basename);
    inode_t ToInode(const fs::path &path_val);
    optional<fs::path> ToPath(inode_t inode) const;
    set<fs::path> ToPaths(inode_t inode) const;

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
