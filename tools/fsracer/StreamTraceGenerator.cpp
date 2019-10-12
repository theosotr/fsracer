#include <cstring>
#include <regex>
#include <stdio.h>
#include <sstream>

#include "Debug.h"
#include "StreamTraceGenerator.h"
#include "Utils.h"


namespace fstrace {


Consumes *EmitConsumes(const std::string &line) {
  std::regex rgx("consumes ([a-zA-Z0-9_:.-]+) (.*)");
  std::smatch groups;
  if (std::regex_search(line, groups, rgx)) {
    return new Consumes(groups[1].str(), groups[2].str());
  }
  return nullptr;
}


Produces *EmitProduces(const std::string &line) {
  std::regex rgx("consumes ([a-zA-Z0-9_:.-]+) (.*)");
  std::smatch groups;
  if (std::regex_search(line, groups, rgx)) {
    return new Produces(groups[1].str(), groups[2].str());
  }
  return nullptr;
}


void StreamTraceGenerator::Start() {
  fp = fopen(trace_file.c_str(), "r");
  if (fp == nullptr) {
    std::stringstream ss;
    ss << strerror(errno);
    debug::err("TraceError") << "Error while opening file "
        + trace_file + ": " + ss.str();
    exit(EXIT_FAILURE);
  }
  has_next = true;
}


TraceNode *StreamTraceGenerator::GetNextTrace() {
  if (!has_next) {
    return nullptr;
  }
  if (getline(&trace_line, &trace_line_len, fp) != -1) {
    return ParseLine(trace_line);
  } else {
    has_next = false;
    return nullptr;
  } 
}


void StreamTraceGenerator::Stop() {
  if (fp != nullptr) {
    fclose(fp);
  }

  if (trace_line) {
    free(trace_line);
  }
}


bool StreamTraceGenerator::HasNext() const {
  return has_next;
}


TraceNode *StreamTraceGenerator::ParseLine(const std::string &line) {
  if (utils::StartsWith(line, "consumes")) {
    return EmitConsumes(line);
  } else if (utils::StartsWith(line, "produces")) {
    return EmitProduces(line);
  } else {
    return nullptr;
  }
}


} // namespace fstrace
