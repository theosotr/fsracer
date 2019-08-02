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
typedef map<pair<inode_t, string>, inode_t> inode_table_t;


class InodeTable : public Table<pair<inode_t, string>, inode_t> {
  public:
    InodeTable():
      next_inode(0) {
        AddEntry(ROOT_INODE, "/");
      };

    void AddEntry(inode_t inode_p, string basename,
                  inode_t inode = ROOT_INODE);
    void RemoveEntry(inode_t inode, string basename);
    optional<inode_t> GetInode(inode_t inode_p, string basename);
    inode_t ToInode(fs::path path_val);
    optional<fs::path> ToPath(inode_t inode);
    set<fs::path> ToPaths(inode_t inode);

  private:
    map<inode_t, set<fs::path>> rev_table;

    size_t next_inode;
};


}


#endif
