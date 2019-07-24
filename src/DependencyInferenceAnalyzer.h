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


class DependencyInferenceAnalyzer : public Analyzer {

  public:
    enum GraphFormat {
      DOT,
      CSV
    };

    DependencyInferenceAnalyzer():
      current_block(nullptr) {  }
    ~DependencyInferenceAnalyzer() {  };

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

    void SaveDependencyGraph(enum GraphFormat graph_format,
                             const string output_file);

  private:

    struct EventInfo {
      size_t event_id;
      Event event;
      set<size_t> dependencies;

      EventInfo(size_t event_id_, Event event_):
        event_id(event_id_),
        event(event_) {  }
    };

    /// The dependency graph of events.
    unordered_map<size_t, EventInfo> dep_graph;
    set<size_t> alive_events;
    Block *current_block;

    void AddAliveEvent(size_t event_id);
    void RemoveAliveEvent(size_t event_id);
    void AddEventInfo(size_t event_id);
    void AddEventInfo(size_t event_id, Event event);
    EventInfo GetEventInfo(size_t event_id);

    // Methods for constructing the dependency graph based on
    // the type of events.
    void ProceedSEvent(EventInfo &new_event, EventInfo &old_event);
    void ProceedMEvent(EventInfo &new_event, EventInfo &old_event);
    void ProceedWEvent(EventInfo &new_event, EventInfo &old_event);
    void ProceedEXTEvent(EventInfo &new_event, EventInfo &old_event);
    void AddDependencies(size_t event_id, Event event);
    void AddDependency(size_t source, size_t target);
    void ConnectWithWEvents(EventInfo event_info);

    // Methods for storing the dependency graph.
    void ToCSV(ostream &stream);
    void ToDot(ostream &stream);
};


}


#endif
