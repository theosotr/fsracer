#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <map>
#include <optional>
#include <utility>
#include <set>



namespace table {

template<class T1, class T2>
class Table {

  public:
    using table_t = std::map<T1, T2>;

    void AddEntry(T1 key, T2 value) {
      table[key] = value;
    }

    std::optional<T2> PopEntry(const T1 &key) {
      std::optional<T2> val;
      typename table_t::iterator it = table.find(key);
      if (it != table.end()) {
        val = it->second;
        table.erase(it);
        return val;
      }
      return val;
    }

    void RemoveEntry(const T1 &key) {
      PopEntry(key);
    }

    std::optional<T2> GetValue(const T1 &key) const {
      std::optional<T2> val;
      typename table_t::const_iterator it = table.find(key);
      if (it != table.end()) {
        val = it->second;
      }
      return val;
    }

    table_t GetTable() const {
      return table;
    }

    typename table_t::iterator begin() {
      // TODO use a custom iterator.
      return table.begin();
    }

    typename table_t::iterator end() {
      return table.end();
    }

    typename table_t::const_iterator begin() const {
      // TODO use a custom iterator.
      return table.begin();
    }

    typename table_t::const_iterator end() const {
      return table.end();
    }

  protected:
    std::map<T1, T2> table;
};


}


#endif
