#ifndef INODE_TABLE_H
#define INODE_TABLE_H

#include <iostream>
#include <map>
#include <optional>
#include <utility>
#include <set>

#define ROOT_INODE 0


using namespace std;


namespace {

typedef size_t inode_t;
typedef map<pair<inode_t, string>, inode_t> inode_table_t;


class InodeTable {
  public:
    InodeTable():
      next_inode(0) {
        AddEntry(ROOT_INODE, "/");
      };

    void AddEntry(inode_t inode_p, string basename,
                  inode_t inode = ROOT_INODE);
    void RemoveEntry(inode_t inode_p, string basename);
    optional<inode_t> GetInode(inode_t inode_p, string basename);
    inode_t ToInode(string filename);
    optional<string> ToPath(inode_t inode);

  private:
    inode_table_t table;
    map<inode_t, set<string>> rev_table;

    size_t next_inode;
};


}


#endif
