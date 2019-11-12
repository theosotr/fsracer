#ifndef INODE_TABLE_H
#define INODE_TABLE_H

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <utility>
#include <set>

#include "Table.h"


#define ROOT_INODE 0


namespace fs = std::filesystem;

namespace table {

using inode_t = size_t;
using inode_key_t = std::pair<inode_t, std::string>;
using inode_table_t = std::map<std::pair<inode_t, std::string>, inode_t>;


class InodeTable : public Table<inode_key_t, inode_t> {
  public:
    InodeTable():
      next_inode(1) {
        InodeTable::AddEntry(ROOT_INODE, "/", "/");
    };

    void AddEntry(inode_t inode_p, std::string basename, std::string p);
    void AddEntry(inode_t inode_p, std::string basename, std::string p,
                  inode_t inode);
    void RemoveEntry(inode_t inode, const std::string &basename);
    std::optional<inode_t> GetInode(inode_t inode_p,
                                    const std::string &basename) const;
    void OpenInode(inode_t inode_p, const std::string &basename);
    void CloseInode(inode_t inode_p, const std::string &basename);
    inode_t ToInode(const fs::path &path_val);
    std::optional<fs::path> ToPath(inode_t inode) const;
    std::set<fs::path> ToPaths(inode_t inode) const;

  private:
    enum INodeType {
      LINKED,
      UNLINKED
    };

    std::map<inode_t, std::set<fs::path>> rev_table;
    Table<inode_key_t, std::pair<enum INodeType, size_t>> open_inodes;

    size_t next_inode;

    void RemoveInode(inode_t inode, std::string basename);
};


}


#endif
