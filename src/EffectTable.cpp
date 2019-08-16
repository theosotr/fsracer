#include "EffectTable.h"


namespace table {


void EffectTable::AddPathEffect(const fs::path &p, FSAccess fs_access) {
  table_t::iterator it = table.find(p);
  if (it != table.end()) {
    it->second.push_back(fs_access);
  } else {
    vector<FSAccess> v;
    v.push_back(fs_access);
    table[p] = v;
  }
}


} // namespace table
