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


size_t Task::GetTaskValue() const {
  return task_value;
}


enum Task::TaskType Task::GetTaskType() const {
  return task_type;
}


std::string Task::ToString() const {
  std::string str = std::to_string(task_value);
  switch (task_type) {
    case S:
      return "S " + str;
    case M:
      return "M " + str;
    case W:
      return "W " + str;
    case MAIN:
      return "MAIN";
    default:
      return "EXTERNAL";
  }
}


std::string NewTask::ToString() const {
  DebugInfo debug_info = GetDebugInfo();
  return "newTask " + task_name + " " + task.ToString() + debug_info.ToString();
}


std::string NewTask::GetTaskName() const {
  return task_name;
}


Task NewTask::GetTask() const {
  return task;
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


std::string DependsOn::GetTarget() const {
  return target;
}


std::string DependsOn::GetSource() const {
  return source;
}


std::string DependsOn::ToString() const {
  return "dependsOn " + target + " " + source;
}


} // namespace fstrace
