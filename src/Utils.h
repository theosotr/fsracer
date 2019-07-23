#include <iostream>


namespace utils {
  std::string PtrToString(void *ptr);

  size_t GetCurrentThread(void *wrapctx);
}
