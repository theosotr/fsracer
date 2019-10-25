#include <assert.h>
#include <ostream>

#include "DependencyInferenceAnalyzer.h"
#include "Graph.h"


namespace graph {


bool TaskDir::IsTask() const {
  return task.has_value();
}


fstrace::Task TaskDir::GetTask() const {
  return task.value();
}

} // namespace graph


namespace analyzer {


DependencyInferenceAnalyzer::DependencyInferenceAnalyzer(
    enum graph::GraphFormat graph_format_):
  in_sysop(false),
  graph_format(graph_format_) {
    task_stack.push("main");
}


std::string DependencyInferenceAnalyzer::GetName() const {
  return "DependencyInferenceAnalyzer";
}


void DependencyInferenceAnalyzer::AnalyzeConsumes(
    const fstrace::Consumes *consumes) {
  if (!consumes) {
    return;
  }
  std::string task_name = consumes->GetTaskName();
  std::string obj = consumes->GetObject();
  dep_graph.AddNode(obj, graph::TaskDir(obj));
  dep_graph.AddEdge(task_name, obj, graph::CONSUMES);
}


void DependencyInferenceAnalyzer::AnalyzeProduces(
    const fstrace::Produces *produces) {
  if (!produces) {
    return;
  }
  std::string task_name = produces->GetTaskName();
  std::string obj = produces->GetObject();
  dep_graph.AddNode(obj, graph::TaskDir(obj));
  dep_graph.AddEdge(task_name, obj, graph::PRODUCES);
}


void DependencyInferenceAnalyzer::AnalyzeDependsOn(
    const fstrace::DependsOn *depends_on) {
  if (!depends_on) {
    return;
  }
  std::string source = depends_on->GetSource();
  std::string target = depends_on->GetTarget();
  dep_graph.AddEdge(source, target, graph::HAPPENS_BEFORE);
}


void DependencyInferenceAnalyzer::AnalyzeNewTask(
    const fstrace::NewTask *new_task) {
  if (!new_task) {
    return;
  }
  std::string task_name = new_task->GetTaskName();
  fstrace::Task task = new_task->GetTask();
  dep_graph.AddNode(task_name, graph::TaskDir(task_name, task));
  assert(!task_stack.empty());
  dep_graph.AddEdge(task_stack.top(), task_name, graph::HAPPENS_BEFORE);
}


void DependencyInferenceAnalyzer::AnalyzeExecTaskBeg(
    const fstrace::ExecTaskBeg *exec_task) {
  if (!exec_task) {
    return;
  }
  std::string task_name = exec_task->GetTaskName();
  dep_graph.AddNodeAttr(task_name, EXECUTED_ATTR);
  task_stack.push(task_name);
}


void DependencyInferenceAnalyzer::AnalyzeSysOpBeg(
    const fstrace::SysOpBeg *sys_op) {
  if (sys_op) {
    in_sysop = true;
  }
}


void DependencyInferenceAnalyzer::AnalyzeEnd(const fstrace::End *end) {
  if (!in_sysop) {
    task_stack.pop();
  } else {
    in_sysop = false;
  }
}


void DependencyInferenceAnalyzer::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  dep_graph.PrintGraph(graph_format, out->OutStream());
  // Clear out object. Cannot be reused.
  delete out;
}


DependencyInferenceAnalyzer::dep_graph_t
DependencyInferenceAnalyzer::GetDependencyGraph() const {
  return dep_graph;
}


} // namespace analyzer
