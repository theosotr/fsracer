#ifndef DEPENDENCY_INFERENCE_ANALYZER_H
#define DEPENDENCY_INFERENCE_ANALYZER_H 

#include <iostream>
#include <unordered_map>
#include <utility>
#include <set>

#include "Analyzer.h"
#include "Graph.h"
#include "Operation.h"
#include "OutWriter.h"
#include "Trace.h"

#define EXECUTED_ATTR "EXECUTED"


using namespace std;
using namespace operation;
using namespace trace;


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
  HAPPENS_BEFORE
};


/**
 * This is a template specilization for printing nodes and edges
 * of a dependency graph of events.
 */
template<>
struct GraphPrinter<Event, enum EdgeLabel> : public GraphPrinterDefault {
  public:
    using NodeInfo = Node<Event, enum EdgeLabel>;
    static string PrintNodeDot(size_t node_id, const NodeInfo &node_info) {
      // If the given node (i.e., event) has not been executed,
      // then omit printing it.
      if (!node_info.HasAttribute(EXECUTED_ATTR)) {
        return "";
      }
      if (node_info.node_obj.GetEventType() == Event::MAIN) {
        // Special treatment on the node corresponding to the main
        // event.
        return to_string(node_id) + "[label=\"MAIN_" +
          to_string(node_id) + "\"]";
      } else {
        string node_str = to_string(node_id);
        return node_str + "[label=\"" + node_str + "["
          + node_info.node_obj.ToString() + "]\"]";
      }
    }

    static string PrintNodeCSV(size_t node_id, const NodeInfo &node_info) {
      // If the given node (i.e., event) has not been executed,
      // then omit printing it.
      if (!node_info.HasAttribute(EXECUTED_ATTR)) {
        return "";
      }
      return to_string(node_info.node_id);
    }

    static string PrintEdgeLabel(enum EdgeLabel label) {
      // Convert edge labels of the dependency graph into a string.
      switch (label) {
        case CREATES:
          return "creates";
        case HAPPENS_BEFORE:
          return "before";
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
    /**
     * The type of the dependency graph.
     *
     * Each node in the dependency graph represents an event.
     * Also, every edge in this graph has a label defined in
     * the `EdgeLabel` enumeration.
     */
    using dep_graph_t = graph::Graph<Event, enum graph::EdgeLabel>;
    /**
     * The type of EventInfo.
     *
     * Each node represents information about an event.
     */
    using EventInfo = dep_graph_t::NodeInfo;

    /** Default Constructor of the analyzer. */
    DependencyInferenceAnalyzer(enum graph::GraphFormat graph_format_):
      current_block(nullptr),
      pending_ev(0),
      current_context(MAIN_BLOCK),
      graph_format(graph_format_)
  {  }
    /** Default Destructor. */
    ~DependencyInferenceAnalyzer() {  };

    /** Display the pretty name of the analyzer. */
    string GetName() const {
      return "DependencyInferenceAnalyzer";
    }

    void Analyze(const TraceNode *trace_node);
    void AnalyzeTrace(const Trace *trace);
    void AnalyzeBlock(const Block *block);
    void AnalyzeExpr(const Expr *expr);
    void AnalyzeSubmitOp(const SubmitOp *submit_op) {  };
    void AnalyzeExecOp(const ExecOp *exec_op) {  };
    void AnalyzeNewEvent(const NewEventExpr *new_ev_expr);
    void AnalyzeLink(const LinkExpr *link_expr);
    void AnalyzeTrigger(const Trigger *trigger_expr);

    void AnalyzeOperation(const Operation *operation) {  }
    void AnalyzeNewFd(const NewFd *new_fd) {  }
    void AnalyzeDelFd(const DelFd *del_fd) {  }
    void AnalyzeHpath(const Hpath *hpath) {  }
    void AnalyzeHpathSym(const HpathSym *hpathsym) {  }
    void AnalyzeLink(const Link *link) {  }
    void AnalyzeRename(const Rename *rename) {  }
    void AnalyzeSymlink(const Symlink *symlink) {  }

    void DumpOutput(writer::OutWriter *out) const;

    const dep_graph_t &GetDependencyGraph() const {
      return dep_graph;
    }

  private:
    /// The dependency graph of events.
    dep_graph_t dep_graph;

    /**
     * The set of alive events (i.e., events whose corresponding callbacks)
     * have not been executed yet.
     */
    set<size_t> alive_events;
    // The block that is currently being processed by the analyzer.
    const Block *current_block;

    /**
     * This is the event that we need to connext with the first
     * created event in the current execution block. */
    size_t pending_ev;
    /**
     * The ID of the event that corresponds to the context where
     * the current block is executed.
     */
    size_t current_context;

    /// Specifies the format of the generated dependency graph.
    enum graph::GraphFormat graph_format;

    // Methods for perfoming operations on the dependency graph and
    // the set of alive events.
    
    /** Adds new event to the set of alive events. */
    void AddAliveEvent(size_t event_id);

    /** Removes the given event from the set of alive events. */
    void RemoveAliveEvent(size_t event_id);

    // Methods for constructing the dependency graph based on
    // the type of events.
    
    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type S.
     */
    void ProceedSEvent(const EventInfo &new_event, const EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type M.
     */
    void ProceedMEvent(const EventInfo &new_event, const EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type W.
     */
    void ProceedWEvent(const EventInfo &new_event, const EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type EXT.
     */
    void ProceedEXTEvent(const EventInfo &new_event, const EventInfo &old_event);

    /**
     * This method processes an event with the given ID and type information.
     *
     * It inspects every existing event, and correlates it with the given one
     * if it is necessary.
     */
    void AddDependencies(size_t event_id, const Event &event);

    /**
     * This method prunes redundant edges between the previous nodes
     * of the given event and its next nodes.
     */
    void PruneEdges(size_t event_id);

    /** Make all the event whose type is W dependent on the given event. */
    void ConnectWithWEvents(const EventInfo &event_info);

    void ConnectSubGraph();
};


}


#endif
