#include <fstream>

#include "driver.hpp"

namespace fstrace {


driver::driver() {
  trace_f = new trace::Trace();
}

driver::~driver() {
  if (trace_f) {
    delete trace_f;
  }

  if (scanner) {
    delete scanner;
  }

  if (m_parser) {
    delete m_parser;
  }
}


int driver::Parse(const std::string &f) {
  std::ifstream in_file (f);
  if (!in_file.good()) {
    exit(EXIT_FAILURE);
  }
  scanner = new Scanner(&in_file);
  m_parser = new parser(*scanner, *this);
  return m_parser->parse();
}


} // namespace fstrace
