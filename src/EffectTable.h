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


struct FSAccess {
  size_t event_id;
  enum Hpath::EffectType effect_type;
  DebugInfo debug_info;

  FSAccess(size_t event_id_, enum Hpath::EffectType effect_type_,
           DebugInfo debug_info_):
    event_id(event_id_),
    effect_type(effect_type_),
    debug_info(debug_info_) {  }
};


class EffectTable :
  public Table<fs::path, vector<FSAccess>> {

    public:
      void AddPathEffect(fs::path p, FSAccess fs_access);

};


} // namespace table

#endif
