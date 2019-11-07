#ifndef DEPENDENCY_INFERENCE_EXP_ANALYZER_H
#define DEPENDENCY_INFERENCE_EXP_ANALYZER_H 

#include <optional>
#include <ostream>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <set>
#include <stack>

#include "Analyzer.h"
#include "Graph.h"
#include "FStrace.h"
#include "Table.h"

#define EXECUTED_ATTR "EXECUTED"


namespace graph {

/**
 * Enumeration that indicates the type of an edge found
 * in the dependency graph.
 */
enum EdgeLabel {
  /// Indicates that the source event creates the target.
  CREATES,
  /**
   * Indicates that the source event happens-before the target;
   * it does not create it though.
   */
  HAPPENS_BEFORE,
  CONSUMES,
  PRODUCES
};


struct TaskDir {
  std::string id;
  std::optional<fstrace::Task> task;

  TaskDir(std::string id_):
    id(id_) {  }
  TaskDir(std::string id_, fstrace::Task task_):
    id(id_),
    task(task_) {  }

  bool IsTask() const;
  fstrace::Task GetTask() const;
};


/**
 * This is a template specilization for printing nodes and edges
 * of a dependency graph of events.
 */
template<>
struct GraphPrinter<TaskDir, enum EdgeLabel> : public GraphPrinterDefault {
public:
  using NodeInfo = Node<TaskDir, enum EdgeLabel>;
  static string PrintNodeDot(string node_id, const NodeInfo &node_info) {
    // If the given node (i.e., event) has not been executed,
    // then omit printing it.
    const TaskDir &task_dir = node_info.node_obj;
    std::string prefix = task_dir.IsTask() ? "task:" : "file:";
    if (task_dir.IsTask() && !node_info.HasAttribute(EXECUTED_ATTR)) {
      return "";
    }
    std::string label = prefix + node_id;
    if (task_dir.IsTask()) {
      label += "[" + task_dir.GetTask().ToString() + "]";
    }
    return "\"" + node_id + "\"" + "[label=\"" + label + "\"]";
  }

  static string PrintNodeCSV(string node_id, const NodeInfo &node_info) {
    // If the given node (i.e., event) has not been executed,
    // then omit printing it.
    if (!node_info.HasAttribute(EXECUTED_ATTR)) {
      return "";
    }
    return node_info.node_id;
  }

  static string PrintEdgeLabel(enum EdgeLabel label) {
    // Convert edge labels of the dependency graph into a string.
    switch (label) {
      case CREATES:
        return "creates";
      case HAPPENS_BEFORE:
        return "before";
      case CONSUMES:
        return "consumes";
      default:
        return "produces";
    }
  }

  static string PrintEdgeDot(const NodeInfo &source, const NodeInfo &target) {
    if (IgnoreEdge(source, target)) {
      return ""; // The edge is not printed.
    }
    return GraphPrinterDefault::PrintEdgeDot(source, target);
  }

  static string PrintEdgeCSV(const NodeInfo &source, const NodeInfo &target) {
    if (IgnoreEdge(source, target)) {
      return ""; // The edge is not printed.
    }
    return GraphPrinterDefault::PrintEdgeCSV(source, target);
  }

private:
  /**
   * If any of the given nodes (either the source or the target node)
   * has not been executed, then this edge should be ignored.
   */
  static bool IgnoreEdge(const NodeInfo &source, const NodeInfo &target) {
    return !source.HasAttribute(EXECUTED_ATTR) ||
      !target.HasAttribute(EXECUTED_ATTR);
  }
};


}


namespace analyzer {


/**
 * An analyzer of traces that infers the dependencies among
 * events.
 */
class DependencyInferenceAnalyzer : public Analyzer {
public:
  using dep_graph_t = graph::Graph<graph::TaskDir, enum graph::EdgeLabel>;
  using DepGNodeInfo = dep_graph_t::NodeInfo;

  /** Default Constructor of the analyzer. */
  DependencyInferenceAnalyzer(enum graph::GraphFormat graph_format_);

  /** Display the pretty name of the analyzer. */
  std::string GetName() const;
  void AnalyzeConsumes(const fstrace::Consumes *consumes);
  void AnalyzeProduces(const fstrace::Produces *produces);
  void AnalyzeNewTask(const fstrace::NewTask *new_task);
  void AnalyzeDependsOn(const fstrace::DependsOn *depends_on);
  void AnalyzeExecTask(const fstrace::ExecTask *exec_task) {  }
  void AnalyzeExecTaskBeg(const fstrace::ExecTaskBeg *exec_task);
  void AnalyzeSysOp(const fstrace::SysOp *sys_op) {  }
  void AnalyzeSysOpBeg(const fstrace::SysOpBeg *sys_op);
  void AnalyzeEnd(const fstrace::End *end);

  void AnalyzeNewFd(const fstrace::NewFd *new_fd) {  }
  void AnalyzeDelFd(const fstrace::DelFd *del_fd) {  }
  void AnalyzeDupFd(const fstrace::DupFd *dup_fd) {  }
  void AnalyzeHpath(const fstrace::Hpath *hpath) { }
  void AnalyzeHpathSym(const fstrace::HpathSym *hpathsym) {  }
  void AnalyzeLink(const fstrace::Link *link) {  }
  void AnalyzeRename(const fstrace::Rename *rename) {  }
  void AnalyzeSymlink(const fstrace::Symlink *symlink) {  }
  void AnalyzeNewProc(const fstrace::NewProc *new_proc) {  }
  void AnalyzeSetCwd(const fstrace::SetCwd *set_cwd) {  }
  void AnalyzeSetCwdFd(const fstrace::SetCwdFd *set_cwdfd) {  }

  void DumpOutput(writer::OutWriter *out) const;
  dep_graph_t GetDependencyGraph() const;

private:
  /// The dependency graph of events.
  dep_graph_t dep_graph;

  std::stack<std::string> task_stack;
  bool in_sysop;
  table::Table<std::string, fstrace::Task> task_info;

  enum graph::GraphFormat graph_format;
};


}


#endif
