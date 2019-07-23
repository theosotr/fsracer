#include <fstream>

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
  AddEventInfo(event_id, new_event->GetEvent());
  // We add the dependency between the event corresponding to the current block
  // and the newly created event.
  AddDependency(block_id, event_id);
  AddDependencies(event_id, new_event->GetEvent());
  // Add the newly-created event to the list of alive events,
  // i.e., events whose corresponding callbacks are pending.
  AddAliveEvent(event_id);
}


void DependencyInferenceAnalyzer::AnalyzeLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  AddDependency(link_expr->GetSourceEvent(), link_expr->GetTargetEvent());
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
        // This event has higher priority than the current one.
        AddDependency(event_info.event_id, event_id);
        break;
      case Event::M:
        switch (event.GetEventType()) {
          case Event::S:
            // The current event has higher priority (S > M).
            AddDependency(event_id, event_info.event_id);
            break;
          case Event::M:
            if (event.GetEventValue() < event_info.event.GetEventValue()) {
              // The current event has higher priority because its
              // event value is smaller. (W i > W j if i < j).
              AddDependency(event_id, event_info.event_id);
            } else {
              AddDependency(event_info.event_id, event_id);
            }
            break;
          case Event::W:
            // The current event has smaller priority (W < M).
            AddDependency(event_info.event_id, event_id);
            break;
        }
        break;
      case Event::W:
        switch (event.GetEventType()) {
          case Event::S:
          case Event::M:
            // The current event has higher priority (S > W and M > W).
            AddDependency(event_id, event_info.event_id);
            break;
          case Event::W:
            if (event.GetEventValue() == event_info.event.GetEventValue()) {
              // We follow a FIFO approach for events of the same type
              // and same event value.
              AddDependency(event_id, event_info.event_id);
            }
            break;
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


void DependencyInferenceAnalyzer::SaveDependencyGraph(enum GraphFormat graph_format,
                                                      const string output_file) {
  ofstream out_stream;
  out_stream.open(output_file);
  switch (graph_format) {
    case DOT:
      ToDot(out_stream);
      break;
    case CSV:
      ToCSV(out_stream);
      break;
  }
  out_stream.close();
}


void DependencyInferenceAnalyzer::ToCSV(ostream &out) {
  for (auto const &entry : dep_graph) {
    EventInfo event_info = entry.second;
    for (auto const &dependency : event_info.dependencies) {
      out << event_info.event_id << "," << dependency << "\n";
    }
  }
}


void DependencyInferenceAnalyzer::ToDot(ostream &out) {
  out << "digraph {\n";
  for (auto const &entry : dep_graph) {
    // TODO Add node labels.
    EventInfo event_info = entry.second;
    for (auto const &dependency : event_info.dependencies) {
      out << event_info.event_id << "->" << dependency << ";\n";
    }
  }
  out << "}\n";
}


}
