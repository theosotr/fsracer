#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <vector>

#include "scanner.hpp"
#include "parser.hpp"

#include "Trace.h"
#include "Operation.h"



namespace fstrace {

class driver {

public:
  driver();
  ~driver();

  trace::Trace *trace_f;
  std::vector<trace::Expr*> exprs;
  std::vector<operation::Operation*> opers;

  int Parse(const std::string& f);

private:
  Scanner *scanner;
  parser *m_parser;
};


} // namespace fstrace


#endif
