#include <iostream>

#include "TraceGeneratorDriver.hpp"


int
main (int argc, char *argv[])
{
  int res = 0;
  string file = argv[1];
  fstrace::TraceGeneratorDriver driver (file);
  driver.Start();
  std::cout << driver.GetTrace()->ToString() << std::endl;
  return res;
}

