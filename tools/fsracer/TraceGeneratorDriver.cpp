#include <cstring>
#include <fstream>
#include <sstream>

#include "TraceGeneratorDriver.hpp"
#include "Utils.h"


namespace fstrace {


void AddOperationDebugInfo(operation::Operation *op,
                           std::vector<std::string> &debug_info) {
  if (!op) {
    return;
  }
  for (auto const &debug_entry : debug_info) {
    if (debug_entry == "failed") {
      op->MarkFailed();
    } else {
      op->SetActualOpName(debug_entry);
    }
  }
}


TraceGeneratorDriver::TraceGeneratorDriver(std::string file_):
  file(file_),
  lexer(nullptr),
  parser(nullptr) {
    trace_f = new trace::Trace();
  }


TraceGeneratorDriver::~TraceGeneratorDriver() {
  if (trace_f) {
    delete trace_f;
  }

  if (lexer) {
    delete lexer;
  }

  if (parser) {
    delete parser;
  }
}


void TraceGeneratorDriver::Start() {
  std::ifstream in_file (file);
  if (!in_file.good()) {
    std::stringstream ss;
    ss << strerror(errno);
    AddError(utils::err::TRACE_ERROR, "Error while opening file "
        + file + ": " + ss.str(), "");
    return;
  }
  lexer = new TraceLexer(&in_file);
  parser = new TraceParser(*lexer, *this);
  parser->parse();
  return;
}


void TraceGeneratorDriver::Stop() {  }


} // namespace fstrace
