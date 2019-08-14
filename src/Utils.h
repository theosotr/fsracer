#ifndef UTILS_H
#define UTILS_H

#include <chrono>
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

/** Class that tracks the time interval between two timer periods. */
class timer {
public:
  /** Start tracking time. */
  void Start();

  /** Stop tracking time. */
  void Stop();

  /** Get time interval in milli seconds. */
  double GetTimeMillis() const;

  /** Get time interval in seconds. */
  double GetTimeSeconds() const;

  /** Get time interval in micro seconds. */
  double GetTimeMicros() const;

private:
  /// Starting time point.
  std::chrono::high_resolution_clock::time_point start_time;
  /// Time interval in micro seconds.
  long time;
};

} //namespace utils


#endif
