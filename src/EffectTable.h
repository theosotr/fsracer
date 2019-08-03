#ifndef EFFECT_TABLE_H
#define EFFECT_TABLE_H


#include <experimental/filesystem>
#include <utility>
#include <vector>

#include "Table.h"
#include "Trace.h"


using namespace std;
using namespace trace;
namespace fs = experimental::filesystem;


namespace table {

class EffectTable :
  public Table<fs::path, vector<pair<size_t, enum Hpath::EffectType>>> {

    public:
      void AddPathEffect(fs::path p, size_t block_id,
                         enum Hpath::EffectType effect);

};


}

#endif
