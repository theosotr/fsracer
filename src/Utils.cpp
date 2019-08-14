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


} // namespace utils
