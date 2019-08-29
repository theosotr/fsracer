#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <vector>

#include "scanner.hpp"
#include "parser.hpp"

#include "Trace.h"
#include "Operation.h"



namespace fstrace {


void AddOperationDebugInfo(operation::Operation *op,
                           std::vector<std::string> &debug_info);


class driver {

public:
  driver();
  ~driver();

  std::vector<trace::Expr*> exprs;
  std::vector<operation::Operation*> opers;

  int Parse(const std::string& f);

  trace::Trace *GetTrace() {
    return trace_f;

  }

  friend class parser;
  friend class Scanner;

private:
  trace::Trace *trace_f;
  Scanner *scanner;
  parser *m_parser;
};


} // namespace fstrace


#endif
