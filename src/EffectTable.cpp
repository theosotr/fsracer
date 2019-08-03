#include "EffectTable.h"


namespace table {


void EffectTable::AddPathEffect(fs::path p, size_t block_id,
    enum Hpath::EffectType effect) {
  table_t::iterator it = table.find(p);
  if (it != table.end()) {
    it->second.push_back({ block_id, effect });
  } else {
    vector<pair<size_t, enum Hpath::EffectType>> v;
    v.push_back({ block_id, effect });
    table[p] = v;
  }
}


}
