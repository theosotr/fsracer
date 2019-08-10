#include <iostream>


namespace utils {
  std::string PtrToString(const void *ptr);

  size_t GetCurrentThread(void *wrapctx);

  std::string GetRightSubstr(std::string &str, std::string delm);
}
