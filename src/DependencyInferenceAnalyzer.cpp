#include <fstream>
#include <set>
#include <tuple>

#include "DependencyInferenceAnalyzer.h"
#include "Operation.h"
#include "Trace.h"


using namespace operation;
using namespace trace;



namespace analyzer {


string DependencyInferenceAnalyzer::LabelToString(enum EdgeLabel label) {
  switch (label) {
    case CREATES:
      return "creates";
    case HAPPENS_BEFORE:
      return "before";
  }
}


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

  size_t block_id = block->GetBlockId();
  current_context = block_id;
  if (block_id != MAIN_BLOCK) {
    EventInfo event_info = GetEventInfo(block_id);
    // If this block corresponds to a W event,
    // we try to associate it with other W events,
    // since now know when it's executed.
    ConnectWithWEvents(event_info);

    // We also prune any redundant edges.
    PruneEdges(block_id);

    // If the current event is active, we can infer that
    // it has been previously executed. Therefore, we have
    // to associate the previous block with the first event
    // that is created inside the current one.
    if (event_info.active) {
      if (current_block) {
        pending_ev = current_block->GetBlockId();
      }

    }
  }

  current_block = block;
  vector<Expr*> exprs = block->GetExprs();
  for (auto const &expr : exprs) {
    AnalyzeExpr(expr);
  }
  RemoveAliveEvent(block->GetBlockId());
  MarkActive(block->GetBlockId());
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

  // There is a pending event that we need to connect with the newly-created
  // event.
  //
  // This pending event came from the same block, but it was created
  // the first time the corresponding block was executed.
  if (pending_ev) {
    AddDependency(pending_ev, event_id, HAPPENS_BEFORE);
    pending_ev = 0;
  }
  // We add the dependency between the event corresponding to the current block
  // and the newly created event.
  //
  // The current block *creates* this event.
  AddDependency(block_id, event_id, CREATES);
  // Create *happens-before* realations between the current event
  // and all the existing ones.
  AddDependencies(event_id, new_event->GetEvent());
  // Add the newly-created event to the list of alive events,
  // i.e., events whose corresponding callbacks are pending.
  AddAliveEvent(event_id);
}


void DependencyInferenceAnalyzer::AnalyzeLink(LinkExpr *link_expr) {
  if (!link_expr) {
    return;
  }
  size_t source_ev = link_expr->GetSourceEvent();
  // If the source event corresponds to the current block
  // or to the current context, we presume that we have already
  // created this dependency.
  if (source_ev == current_block->GetBlockId() ||
      source_ev == current_context) {
    return;
  }
  AddDependency(source_ev, link_expr->GetTargetEvent(), HAPPENS_BEFORE);
}


void
DependencyInferenceAnalyzer::AnalyzeTrigger(Trigger *trigger_expr) {
  if (!trigger_expr) {
    return;
  }
  current_context = trigger_expr->GetEventId();
}


void DependencyInferenceAnalyzer::MarkActive(size_t event_id) {
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(event_id);
  if (it != dep_graph.end()) {
    it->second.active = true;
  }
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
  AddEventInfo(event_id, construct_default_event());
}


void DependencyInferenceAnalyzer::AddEventInfo(size_t event_id, Event event) {
  EventInfo event_info = EventInfo(event_id, event);
  dep_graph.emplace(event_id, event_info);
}


DependencyInferenceAnalyzer::EventInfo
DependencyInferenceAnalyzer::GetEventInfo(size_t event_id) {
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(event_id);
  if (it == dep_graph.end()) {
    EventInfo ev = EventInfo(0, Event(Event::S, 0));
    return ev;
  } else {
    return it->second;
  }
} 


void DependencyInferenceAnalyzer::ProceedSEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  AddDependency(old_event.event_id, new_event.event_id, HAPPENS_BEFORE);
}


void DependencyInferenceAnalyzer::ProceedMEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  switch (new_event.event.GetEventType()) {
    case Event::S:
      // The current event has higher priority (S > M).
      AddDependency(new_event.event_id, old_event.event_id, HAPPENS_BEFORE);
      break;
    case Event::M:
      if (new_event.event.GetEventValue() < old_event.event.GetEventValue()) {
        // The current event has higher priority because its
        // event value is smaller. (W i > W j if i < j).
        AddDependency(new_event.event_id, old_event.event_id, HAPPENS_BEFORE);
      } else {
        AddDependency(old_event.event_id, new_event.event_id, HAPPENS_BEFORE);
      }
      break;
    case Event::W:
    case Event::EXT:
      // The current event has smaller priority (W < M).
      AddDependency(old_event.event_id, new_event.event_id, HAPPENS_BEFORE);
  }
}


void DependencyInferenceAnalyzer::ProceedWEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  switch (new_event.event.GetEventType()) {
    case Event::S:
    case Event::M:
      AddDependency(new_event.event_id, old_event.event_id, HAPPENS_BEFORE);
      break;
    case Event::EXT:
      // The new event is external.
      // Therefore, already created W events have higher priority than
      // the external events.
      //
      // XXX: Revisit
      if (old_event.event.GetEventValue() != 0) {
        AddDependency(old_event.event_id, new_event.event_id, HAPPENS_BEFORE);
      }
    default:
      break;
  }
}


void DependencyInferenceAnalyzer::ProceedEXTEvent(EventInfo &new_event,
                                                  EventInfo &old_event) {
  switch (new_event.event.GetEventType()) {
    case Event::S:
    case Event::M:
      // The current event has higher priority (S > W and M > W).
      AddDependency(new_event.event_id, old_event.event_id, HAPPENS_BEFORE);
    default:
      break;
  }
}


void DependencyInferenceAnalyzer::AddDependencies(size_t event_id, Event event) {
  for (auto alive_ev_id : alive_events) {
    unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(alive_ev_id);
    // We ignore events that are not present to the dependency graph.
    // events identical to the current block,
    // as well as inactive events.
    if (it == dep_graph.end() ||
        current_block->GetBlockId() == it->second.event_id) {
      continue;
    }
    EventInfo event_info = it->second;
    EventInfo new_event_info = EventInfo(event_id, event);
    switch (event_info.event.GetEventType()) {
      case Event::S:
        ProceedSEvent(new_event_info, event_info);
        break;
      case Event::M:
        ProceedMEvent(new_event_info, event_info);
        break;
      case Event::W:
        ProceedWEvent(new_event_info, event_info);
        break;
      case Event::EXT:
        ProceedEXTEvent(new_event_info, event_info);
        break;
    }
  }
}


void DependencyInferenceAnalyzer::ConnectWithWEvents(EventInfo event_info) {
  switch (event_info.event.GetEventType()) {
    case Event::S:
    case Event::M:
    case Event::EXT:
      // This event does not have a W type, so we do nothing.
      break;
    case Event::W:
      for (auto alive_ev_id : alive_events) {
        unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(alive_ev_id);
        if (it == dep_graph.end()) {
          continue;
        }
        // In this loop, we get the list of all the alive events
        // whose type is W and their event value is identical with
        // that of the current event.
        //
        // Then, for every event X found in the previous list,
        // we create the following dependency:
        //
        // current_event -> X.
        EventInfo ei = it->second;
        switch (ei.event.GetEventType()) {
          case Event::S:
          case Event::M:
          case Event::EXT:
            break;
          case Event::W:
            if (ei.event.GetEventValue() == event_info.event.GetEventValue()) {
              AddDependency(event_info.event_id, ei.event_id, HAPPENS_BEFORE);
            }
          break;
        }
     }
  }
}


void DependencyInferenceAnalyzer::AddDependency(size_t source, size_t target,
                                                enum EdgeLabel label) {
  if (source == target) {
    return;
  }
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(source);
  if (it != dep_graph.end()) {
    it->second.dependents.insert({ target, label });
    unordered_map<size_t, EventInfo>::iterator target_it = dep_graph.find(target);
    if (target_it == dep_graph.end()) {
      return;
    }
    switch (label) {
      case HAPPENS_BEFORE:
      case CREATES:
        it->second.before.insert(target);
        target_it->second.after.insert(source);
        break;
    }
  }
}


void DependencyInferenceAnalyzer::RemoveDependency(size_t source, size_t target) {
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(source);
  if (it != dep_graph.end()) {
    it->second.dependents.erase({ target, HAPPENS_BEFORE });
  }
}


void DependencyInferenceAnalyzer::PruneEdges(size_t event_id) {
  unordered_map<size_t, EventInfo>::iterator it = dep_graph.find(event_id);
  if (it == dep_graph.end()) {
    return;
  }
  // Iterate over the next nodes of the given event.
  for (auto next : it->second.before) {
    unordered_map<size_t, EventInfo>::iterator next_it = dep_graph.find(next);
    if (next_it == dep_graph.end()) {
      continue;
    }
    // Iterate over the previous nodes of the the given event.
    //
    // If prev is included in the set of previous nodes of next,
    // we remove the corresponding edge, because `prev` is connected with
    // `next` via the given event.
    for (auto prev : it->second.after) {
      if (next_it->second.after.find(prev) != next_it->second.after.end()) {
        RemoveDependency(prev, next);
      }
    }
  }
}


void DependencyInferenceAnalyzer::DumpDependencyGraph(enum GraphFormat graph_format) {
  if (!out) {
    return;
  }
  switch (graph_format) {
    case DOT:
      ToDot(GetOutStream());
      break;
    case CSV:
      ToCSV(GetOutStream());
      break;
  }
}


void DependencyInferenceAnalyzer::ToCSV(ostream &out) {
  // We store the graph in uniform form,
  // the edge list is sorted by on the value of
  // each source and target.
  //
  // The order is ascending.
  set<tuple<size_t, size_t, enum EdgeLabel>> edges;
  for (auto const &entry : dep_graph) {
    EventInfo event_info = entry.second;
    if (!event_info.active) {
      continue;
    }
    for (auto const &dependent: event_info.dependents) {
      if (GetEventInfo(dependent.first).active) {
        edges.insert(make_tuple(event_info.event_id, dependent.first,
                                dependent.second));
      }
    }
  }
  for (auto const &edge : edges) {
    out << get<0>(edge)
      << ","
      << get<1>(edge)
      << ","
      << LabelToString(get<2>(edge))
      << "\n";
  }
}


void DependencyInferenceAnalyzer::ToDot(ostream &out) {
  out << "digraph {\n";
  for (auto const &entry : dep_graph) {
    EventInfo event_info = entry.second;
    if (!event_info.active) {
      continue;
    }
    if (event_info.event_id == MAIN_BLOCK) {
      out << MAIN_BLOCK << "[label=\"MAIN\"];\n";
    } else {
      out
        << event_info.event_id
        << "[label=\"" << event_info.event_id
        << "["
        << event_info.event.ToString()
        << "]"
        << "\"];\n";
    }
    for (auto const &dependent : event_info.dependents) {
      if (GetEventInfo(dependent.first).active) {
        out << event_info.event_id << "->"
          << dependent.first
          << "[label=\""
          << LabelToString(dependent.second)
          << "\"];\n";
      }
    }
  }
  out << "}\n";
}


}
