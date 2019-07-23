#include "DependencyInferenceAnalyzer.h"
#include "Operation.h"
#include "Trace.h"


using namespace operation;
using namespace trace;

namespace analyzer {


void DependencyInferenceAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    trace_node->Accept(this);
  }
}


void DependencyInferenceAnalyzer::AnalyzeTrace(Trace *trace) {
  if (!trace) {
    return;
  }

  vector<Block*> blocks = trace->GetBlocks();
  for (auto const &block : blocks) {
    AnalyzeBlock(block);
  }
}


void DependencyInferenceAnalyzer::AnalyzeBlock(Block *block) {
  if (!block) {
    return;
  }

  current_block = block;
  vector<Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
    RemoveAliveEvent(block->GetBlockId());
  }
}


void DependencyInferenceAnalyzer::AnalyzeExpr(Expr *expr) {
  if (expr) {
    expr->Accept(this);
  }
}


void DependencyInferenceAnalyzer::AnalyzeNewEvent(NewEventExpr *new_event) {
  if (!new_event) {
    return;
  }

  size_t block_id = current_block->GetBlockId();
  if (block_id == MAIN_BLOCK) {
    // This is the MAIN block, so we add it to the dependency graph
    // since there is not any preceding "newEvent" construct associated with
    // the ID of the current block.
    AddEventInfo(block_id);
  }

  size_t event_id = new_event->GetEventId();
  // Add the newly-created event to the list of alive events,
  // i.e., events whose corresponding callbacks are pending.
  AddAliveEvent(event_id);
  AddEventInfo(event_id, new_event->GetEvent());
  // We add the dependency between the event corresponding to the current block
  // and the newly created event.
  AddDependency(block_id, event_id);
}


void DependencyInferenceAnalyzer::AddAliveEvent(size_t event_id) {
  alive_events.insert(event_id);
}


void DependencyInferenceAnalyzer::RemoveAliveEvent(size_t event_id) {
  set<size_t>::iterator it = alive_events.find(event_id);
  if (it != alive_events.end()) {
    alive_events.erase(it);
  }
}


void DependencyInferenceAnalyzer::AddEventInfo(size_t event_id) {
  AddEventInfo(event_id, ConstructDefaultEvent());
}


void DependencyInferenceAnalyzer::AddEventInfo(size_t event_id, Event event) {
  EventInfo event_info = EventInfo(event_id, event);
  dep_graph.emplace(event_id, event_info);
}


Event DependencyInferenceAnalyzer::ConstructDefaultEvent() {
  return Event(Event::S, 0);
}


void DependencyInferenceAnalyzer::AddDependencies(size_t event_id, Event event) {
  for (auto alive_ev_id : alive_events) {
    unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(alive_ev_id);
    if (it == dep_graph.end()) {
      continue;
    }
    EventInfo event_info = it->second;
    switch (event_info.event.GetEventType()) {
      case Event::S:
        // This is an event with higher priority.
        AddDependency(event_info.event_id, event_id);
        break;
      case Event::M:
        switch (event.GetEventType()) {
          case Event::S:
            AddDependency(event_id, event_info.event_id);
          default:
            if (event.GetEventValue() < event_info.event.GetEventValue()) {
              AddDependency(event_id, event_info.event_id);
            } else {
              AddDependency(event_id, event_info.event_id);
            }
        }
        break;
      case Event::W:
        switch (event.GetEventType()) {
          case Event::S:
            AddDependency(event_id, event_info.event_id);
          default:
            if (event.GetEventValue() == event_info.event.GetEventValue()) {
              AddDependency(event_id, event_info.event_id);
            }
        }
        break;
    }
  }
}


void DependencyInferenceAnalyzer::AddDependency(size_t source, size_t target) {
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(source);
  if (it != dep_graph.end()) {
    it->second.dependencies.insert(target);
  }
}


}
