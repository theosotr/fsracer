#ifndef UTILS_H
#define UTILS_H

#include <execinfo.h>
#include <chrono>
#include <iostream>
#include <ostream>
#include <utility>
#include <vector>


namespace utils {


std::string PtrToString(const void *ptr);

std::string GetRightSubstr(std::string &str, std::string delm);

bool StartsWith(const std::string &str, std::string prefix);

bool IsNumber(const std::string &str);

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


namespace err {


enum ErrType {
  RUNTIME,
  TRACE_ERROR,
  ANALYZER_ERROR
};


inline std::string ErrToString(enum ErrType err_type);


class Error {
public:
  Error(enum ErrType err_type_);
  Error(enum ErrType err_type_, std::string msg_);
  Error(enum ErrType err_type_, std::string msg_, std::string location_);
  Error(enum ErrType err_type_, std::string msg_, std::string desc,
        std::string location_);

  std::string ToString() const;
  friend std::ostream &operator<<(std::ostream &os, const Error &error) {
    os << error.ToString();
    return os;
  }

private:
  enum ErrType err_type;
  std::string msg;
  std::string desc;
  std::string location;
};


} // namespace err

} //namespace utils


#endif
