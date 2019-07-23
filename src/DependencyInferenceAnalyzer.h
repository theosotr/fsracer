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


namespace analyzer {


class DependencyInferenceAnalyzer : public Analyzer {

  public:
    DependencyInferenceAnalyzer():
      current_block(nullptr) {  }
    ~DependencyInferenceAnalyzer();

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

    Event ConstructDefaultEvent();
    void AddAliveEvent(size_t event_id);
    void RemoveAliveEvent(size_t event_id);
    void AddEventInfo(size_t event_id);
    void AddEventInfo(size_t event_id, Event event);
    void AddDependencies(size_t event_id, Event event);
    void AddDependency(size_t source, size_t target); 

};


}


#endif
