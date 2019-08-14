#ifndef UTILS_H
#define UTILS_H


#include <iostream>
#include <utility>
#include <vector>


namespace utils {


  std::string PtrToString(const void *ptr);

  size_t GetCurrentThread(void *wrapctx);

  std::string GetRightSubstr(std::string &str, std::string delm);

  template<template<typename> class C, typename T>
  std::vector<std::pair<T, T>> Get2Combinations(const C<T> &a) {
    std::vector<std::pair<T, T>> combs;
    for (typename C<T>::const_iterator it = a.begin(); it != a.end(); it++) {
      for (auto it_c = it; it_c != a.end(); it_c++) {
        combs.push_back({ *it, *it_c });
      }
    }
    return combs;
  }


} //namespace utils

#endif
