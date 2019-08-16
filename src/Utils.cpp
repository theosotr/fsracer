#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "dr_api.h"
#include "drwrap.h"

#include "Utils.h"


namespace utils {


std::string PtrToString(const void *ptr) {
  std::stringstream ss;
  ss << ptr;
  return ss.str();
}


size_t GetCurrentThread(void *wrapctx) {
  void *drcontext = drwrap_get_drcontext(wrapctx);
  return dr_get_thread_id(drcontext);
}


std::string GetRightSubstr(std::string &str, std::string delm) {
  size_t pos = str.find(delm);
  if (pos == std::string::npos) {
    return str;
  }
  return str.substr(pos + 1, std::string::npos);
}


void timer::Start() {
  start_time = std::chrono::high_resolution_clock::now();
}


void timer::Stop() {
  auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
  time = std::chrono::duration_cast<std::chrono::microseconds>(
      elapsed).count();
}


double timer::GetTimeMillis() const {
  return (double) time / 1000;
}


double timer::GetTimeSeconds() const {
  return (double) time / 1000000;
}


double timer::GetTimeMicros() const {
  return time;
}


namespace err {


std::string ErrToString(enum ErrType err_type) {
  switch (err_type) {
    case RUNTIME:
      return "Runtime Error";
    case TRACE_ERROR:
      return "Trace Error";
    case ANALYZER_ERROR:
      return "Analyzer Error";
  }
}


Error::Error(enum ErrType err_type_):
  err_type(err_type_) {  }


Error::Error(enum ErrType err_type_, std::string msg_):
  err_type(err_type_),
  msg(msg_) {  }


Error::Error(enum ErrType err_type_, std::string msg_, std::string location_):
  err_type(err_type_),
  msg(msg_),
  location(location_) {  }


Error::Error(enum ErrType err_type_, std::string msg_, std::string desc_,
             std::string location_):
  err_type(err_type_),
  msg(msg_),
  desc(desc_),
  location(location_) {  }


std::string Error::ToString() const {
  std::string str = desc + "\n" + ErrToString(err_type);
  if (msg != "") {
    str += ": " + msg;
  }
  if (location != "") {
    str += " (loc: " + location + ")";
  }
  return str;
}


}// namespace err

} // namespace utils
