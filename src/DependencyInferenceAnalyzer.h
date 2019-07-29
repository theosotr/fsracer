#ifndef DEPENDENCY_INFERENCE_ANALYZER_H
#define DEPENDENCY_INFERENCE_ANALYZER_H 

#include <iostream>
#include <unordered_map>
#include <utility>
#include <set>

#include "Analyzer.h"
#include "Operation.h"
#include "OutWriter.h"
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
     * This struct holds all the information associated with an event.
     */
    struct EventInfo {
      /// The ID of the event.
      size_t event_id;
      // Type information of the event.
      Event event;
      // Set of the events dependent on the current one.
      set<pair<size_t, enum EdgeLabel>> dependents;
      /// True if the event is active; false otherwise.
      bool active;

      EventInfo(size_t event_id_, Event event_):
        event_id(event_id_),
        event(event_),
        active(false)
      {  }
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
    DependencyInferenceAnalyzer(writer::OutWriter::WriteOption write_option,
                                string filename):
      Analyzer(write_option, filename),
      current_block(nullptr),
      current_context(MAIN_BLOCK) {  }
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
    void AnalyzeTrigger(Trigger *trigger_expr);

    void AnalyzeNewFd(NewFd *new_fd) {  }
    void AnalyzeDelFd(DelFd *del_fd) {  }
    void AnalyzeHpath(Hpath *hpath) {  }
    void AnalyzeHpath(HpathSym *hpathsym) {  }
    void AnalyzeLink(Link *link) {  }
    void AnalyzeRename(Rename *rename) {  }
    void AnalyzeSymlink(Symlink *symlink) {  }

    /**
     * This method dumps the constructed dependency graph
     * in the specified format (either DOT or CSV).
     */
    void DumpDependencyGraph(enum GraphFormat graph_format);

    /** Converts the edge label to string. */
    static string LabelToString(enum EdgeLabel);

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

    /**
     * The ID of the event that corresponds to the context where
     * the current block is executed.
     */
    size_t current_context;

    // Methods for perfoming operations on the dependency graph and
    // the set of alive events.
    
    /** Marks the given event as active. */
    void MarkActive(size_t event_id);
    
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
    void AddDependency(size_t source, size_t target, enum EdgeLabel label);

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
