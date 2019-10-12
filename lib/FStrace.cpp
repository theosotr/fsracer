#include "FStrace.h"


namespace fstrace {


void DebugInfo::AddDebugInfo(std::string debug) {
  debug_info.push_back(debug);
}


std::string DebugInfo::ToString() const {
  std::string str = "";
  for (auto const &debug : debug_info) {
    str += " !" + debug;
  }
  return str;
}


std::string Consumes::GetTaskName() const {
  return task_name;
}


std::string Consumes::GetObject() const {
  return object;
}


std::string Consumes::ToString() const {
  return "consumes " + task_name + " " + object;
}


std::string Produces::GetTaskName() const {
  return task_name;
}


std::string Produces::GetObject() const {
  return object;
}


std::string Produces::ToString() const {
  return "produces " + task_name + " " + object;
}


} // namespace fstrace
