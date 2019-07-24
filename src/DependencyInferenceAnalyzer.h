#ifndef DEPENDENCY_INFERENCE_ANALYZER_H
#define DEPENDENCY_INFERENCE_ANALYZER_H 

#include <iostream>
#include <unordered_map>
#include <set>

#include "Analyzer.h"
#include "Operation.h"
#include "Trace.h"


using namespace std;
using namespace operation;
using namespace trace;


inline Event
construct_default_event() {
  return Event(Event::S, 0);
}


namespace analyzer {


/**
 * An analyzer of traces that infers the dependencies among
 * events.
 */
class DependencyInferenceAnalyzer : public Analyzer {

  public:
    /**
     * This struct holds all the information associated with an event.
     */
    struct EventInfo {
      /// The ID of the event.
      size_t event_id;
      // Type information of the event.
      Event event;
      // Set of its dependencies.
      set<size_t> dependencies;

      EventInfo(size_t event_id_, Event event_):
        event_id(event_id_),
        event(event_) {  }
    };

    /**
     * Supported formats of the dependency graph.
     *
     * DOT: The dependency graph is represented as a graphviz graph.
     * CSV: The dependency graph is represented as csv file that contains
     *      its edge list.
     */
    enum GraphFormat {
      DOT,
      CSV
    };

    /** Default Constructor of the analyzer. */
    DependencyInferenceAnalyzer():
      current_block(nullptr) {  }
    /** Default Destructor. */
    ~DependencyInferenceAnalyzer() {  };

    /** Display the pretty name of the analyzer. */
    string GetName() {
      return "DependencyInferenceAnalyzer";
    }

    void Analyze(TraceNode *trace_node);
    void AnalyzeTrace(Trace *trace);
    void AnalyzeBlock(Block *block);
    void AnalyzeExpr(Expr *expr);
    void AnalyzeSubmitOp(SubmitOp *submit_op) {  };
    void AnalyzeExecOp(ExecOp *exec_op) {  };
    void AnalyzeNewEvent(NewEventExpr *new_ev_expr);
    void AnalyzeLink(LinkExpr *link_expr);

    void AnalyzeNewFd(NewFd *new_fd) {  }
    void AnalyzeDelFd(DelFd *del_fd) {  }
    void AnalyzeHpath(Hpath *hpath) {  }
    void AnalyzeHpath(HpathSym *hpathsym) {  }
    void AnalyzeLink(Link *link) {  }
    void AnalyzeRename(Rename *rename) {  }
    void AnalyzeSymlink(Symlink *symlink) {  }

    /**
     * This method saves the constructed dependency graph to the given
     * file in the specified format (either DOT or csv).
     */
    void SaveDependencyGraph(enum GraphFormat graph_format,
                             const string output_file);

  private:

    /// The dependency graph of events.
    unordered_map<size_t, EventInfo> dep_graph;
    /**
     * The set of alive events (i.e., events whose corresponding callbacks)
     * have not been executed yet.
     */
    set<size_t> alive_events;
    // The block that is currently being processed by the analyzer.
    Block *current_block;

    // Methods for perfoming operations on the dependency graph and
    // the set of alive events.
    
    /** Adds new event to the set of alive events. */
    void AddAliveEvent(size_t event_id);

    /** Removes the given event from the set of alive events. */
    void RemoveAliveEvent(size_t event_id);

    /**
     * Adds the given event to the dependency graph.
     *
     * Since the type of the event is not given,
     * the type information related to this event are the default ones.
     */
    void AddEventInfo(size_t event_id);

    /** Ads the given event to the dependency graph. */
    void AddEventInfo(size_t event_id, Event event);

    /** Retrieves the information about the given event. */
    EventInfo GetEventInfo(size_t event_id);

    // Methods for constructing the dependency graph based on
    // the type of events.
    
    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type S.
     */
    void ProceedSEvent(EventInfo &new_event, EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type M.
     */
    void ProceedMEvent(EventInfo &new_event, EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type W.
     */
    void ProceedWEvent(EventInfo &new_event, EventInfo &old_event);

    /**
     * This method checks whether the new event is dependent on
     * the old one, and vice versa.
     *
     * The old_event has type EXT.
     */
    void ProceedEXTEvent(EventInfo &new_event, EventInfo &old_event);

    /**
     * This method processes an event with the given ID and type information.
     *
     * It inspects every existing event, and correlates it with the given one
     * if it is necessary.
     */
    void AddDependencies(size_t event_id, Event event);

    /**
     * Makes the target event dependent on the source.
     *
     * source -> target
     */
    void AddDependency(size_t source, size_t target);

    /** Make all the event whose type is W dependent on the given event. */
    void ConnectWithWEvents(EventInfo event_info);

    // Methods for storing the dependency graph.
    
    /** Converts the dependency graph to a CSV edge list format. */
    void ToCSV(ostream &stream);

    /** Converts the dependency graph to a DOT format. */
    void ToDot(ostream &stream);
};


}


#endif