#include <cstring>
#include <regex>
#include <stdio.h>
#include <sstream>

#include "Debug.h"
#include "StreamTraceGenerator.h"
#include "Utils.h"


namespace fstrace {
// TODO Handle parser errors


Consumes *EmitConsumes(const std::string &line) {
  std::regex rgx("consumes ([a-zA-Z0-9_:.-]+) (.*)");
  std::smatch groups;
  if (std::regex_search(line, groups, rgx)) {
    return new Consumes(groups[1].str(), groups[2].str());
  }
  return nullptr;
}


Produces *EmitProduces(const std::string &line) {
  std::regex rgx("consumes ([a-zA-Z0-9_:.-]+) ([^\n]+)");
  std::smatch groups;
  if (std::regex_search(line, groups, rgx)) {
    return new Produces(groups[1].str(), groups[2].str());
  }
  return nullptr;
}


NewTask *EmitNewTask(const std::string &line) {
  std::regex rgx("newTask ([a-zA-Z0-9_:.-]+) ([A-Z]+)([ ]([0-9]))?");
  std::smatch groups;
  if (!std::regex_search(line, groups, rgx)) {
    return nullptr;
  }
  std::string task_name = groups[1].str();
  std::string task_type = groups[2].str();
  if (task_type == "EXTERNAL") {
    return new NewTask(task_name, Task(Task::EXT, 0));
  }
  size_t group_size = 5;
  size_t task_value_group = 4;
  if (groups.size() != group_size) {
    return nullptr;
  }
  if (task_type == "S") {
    return new NewTask(
      task_name, Task(Task::S, std::stoi(groups[task_value_group].str())));
  } else if (task_type == "M") {
    return new NewTask(
      task_name, Task(Task::M, std::stoi(groups[task_value_group].str())));
  } else if (task_type == "W") {
    return new NewTask(
      task_name, Task(Task::W, std::stoi(groups[task_value_group].str())));
  }
  return nullptr;
}


DependsOn *EmitDependsOn(const std::string &line) {
  std::regex rgx("dependsOn ([a-zA-Z0-9_:.-]+) ([a-zA-Z0-9_:.-]+)");
  std::smatch groups;
  if (!std::regex_search(line, groups, rgx)) {
    return nullptr;
  }
  return new DependsOn(groups[1], groups[2]);
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
  } else if (utils::StartsWith(line, "newTask")) {
    return EmitNewTask(line);
  } else if (utils::StartsWith(line, "dependsOn")) {
    return EmitDependsOn(line);
  } else {
    return nullptr;
  }
}


} // namespace fstrace
