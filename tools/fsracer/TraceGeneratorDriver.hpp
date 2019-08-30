#ifndef DRIVER_H
#define DRIVER_H

#include <string>
#include <vector>

#include "TraceLexer.hpp"
#include "TraceParser.hpp"

#include "Trace.h"
#include "TraceGenerator.h"
#include "Operation.h"



namespace fstrace {


void AddOperationDebugInfo(operation::Operation *op,
                           std::vector<std::string> &debug_info);


class TraceGeneratorDriver : public trace_generator::TraceGenerator {

public:
  TraceGeneratorDriver(std::string file_);
  ~TraceGeneratorDriver();

  void Start();

  void Stop();

  std::string GetName() const {
    return "TraceParser";
  }

  trace::Trace *GetTrace() {
    return trace_f;
  }

  friend class TraceParser;

private:
  std::string file;
  TraceLexer *lexer;
  TraceParser *parser;

  std::vector<trace::Expr*> exprs;
  std::vector<operation::Operation*> opers;
};


} // namespace fstrace


#endif
