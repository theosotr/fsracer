#include <iostream>
#include <sstream>
#include <string>

#include "dr_api.h"
#include "drwrap.h"


namespace utils {

  std::string PtrToString(void *ptr) {
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
    return str.substr(pos, std::string::npos);
  }

}
