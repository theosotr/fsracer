#include <iostream>
#include "driver.hpp"


int
main (int argc, char *argv[])
{
  int res = 0;
  fstrace::driver driver;
  if (driver.Parse(argv[1])) {
    exit(EXIT_FAILURE);
  }
  std::cout << driver.GetTrace()->ToString() << std::endl;
  return res;
}

