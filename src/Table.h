#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <map>
#include <optional>
#include <utility>
#include <set>


using namespace std;


namespace table {

template<class T1, class T2>
class Table {
  typedef map<T1, T2> table_t;

  public:
    void AddEntry(T1 key, T2 value) {
      table[key] = value;
    }

    T2 PopEntry(T1 key) {
      typename table_t::iterator it = table.find(key);
      if (it != table.end()) {
        T2 val = it->second;
        table.erase(it);
        return val;
      }
    }

    void RemoveEntry(T1 key) {
      PopEntry(key);
    }

    optional<T2> GetValue(T1 key) {
      optional<T2> val;
      typename table_t::iterator it = table.find(key);
      if (it != table.end()) {
        val = it->second;
      }
      return val;
    }

  protected:
    map<T1, T2> table;
};


}


#endif
