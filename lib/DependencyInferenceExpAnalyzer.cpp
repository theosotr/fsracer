#include <assert.h>
#include <ostream>

#include "DependencyInferenceExpAnalyzer.h"


namespace graph {


bool TaskDir::IsTask() const {
  return task.has_value();
}


fstrace::Task TaskDir::GetTask() const {
  return task.value();
}

} // namespace graph


namespace analyzer {


std::string DependencyInferenceAnalyzerExp::GetName() const {
  return "DependencyInferenceAnalyzer";
}


void DependencyInferenceAnalyzerExp::AnalyzeConsumes(
    const fstrace::Consumes *consumes) {
  if (!consumes) {
    return;
  }
  std::string task_name = consumes->GetTaskName();
  std::string obj = consumes->GetObject();
  dep_graph.AddNode(obj, graph::TaskDir(obj));
  dep_graph.AddEdge(task_name, obj, graph::CONSUMES);
}


void DependencyInferenceAnalyzerExp::AnalyzeProduces(
    const fstrace::Produces *produces) {
  if (!produces) {
    return;
  }
  std::string task_name = produces->GetTaskName();
  std::string obj = produces->GetObject();
  dep_graph.AddNode(obj, graph::TaskDir(obj));
  dep_graph.AddEdge(task_name, obj, graph::PRODUCES);
}


void DependencyInferenceAnalyzerExp::AnalyzeDependsOn(
    const fstrace::DependsOn *depends_on) {
  if (!depends_on) {
    return;
  }
  std::string source = depends_on->GetSource();
  std::string target = depends_on->GetTarget();
  dep_graph.AddEdge(source, target, graph::HAPPENS_BEFORE);
}


void DependencyInferenceAnalyzerExp::AnalyzeNewTask(
    const fstrace::NewTask *new_task) {
  if (!new_task) {
    return;
  }
  std::string task_name = new_task->GetTaskName();
  fstrace::Task task = new_task->GetTask();
  dep_graph.AddNode(task_name, graph::TaskDir(task_name, task));
}


void DependencyInferenceAnalyzerExp::AnalyzeExecTaskBeg(
    const fstrace::ExecTaskBeg *exec_task) {
  if (!exec_task) {
    return;
  }
  std::string task_name = exec_task->GetTaskName();
  dep_graph.AddNodeAttr(task_name, EXECUTED_ATTR);
}


void DependencyInferenceAnalyzerExp::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  dep_graph.PrintGraph(graph_format, out->OutStream());
  // Clear out object. Cannot be reused.
  delete out;
}


} // namespace analyzer
