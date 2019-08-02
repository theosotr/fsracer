#ifndef FS_ANALYZER_H
#define FS_ANALYZER_H

#include <experimental/filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "Analyzer.h"
#include "InodeTable.h"
#include "Table.h"
#include "Trace.h"


using namespace std;
using namespace analyzer;
using namespace table;
using namespace trace;

namespace fs = experimental::filesystem;


namespace analyzer {

class FSAnalyzer : public Analyzer {
  typedef size_t proc_t;
  typedef size_t addr_t;
  typedef size_t fd_t;

  private:
    Table<addr_t, fs::path> cwd_table;
    Table<pair<addr_t, fd_t>, inode_t> fd_table;
    Table<inode_t, fs::path> symlink_table;
    Table<proc_t, pair<addr_t, addr_t>> proc_table;
    InodeTable inode_table;

    Block *current_block;
    size_t main_process;

};


}
#endif
