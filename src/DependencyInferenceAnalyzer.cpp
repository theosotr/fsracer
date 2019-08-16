#include <fstream>
#include <set>
#include <tuple>

#include "DependencyInferenceAnalyzer.h"
#include "Operation.h"
#include "Trace.h"


using namespace operation;
using namespace trace;



namespace analyzer {


void DependencyInferenceAnalyzer::Analyze(TraceNode *trace_node) {
  if (trace_node) {
    analysis_time.Start();
    trace_node->Accept(this);
    analysis_time.Stop();
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
    optional<EventInfo> event_info = dep_graph.GetNodeInfo(block_id);
    if (!event_info.has_value()) {
      return;
    }
    // If this block corresponds to a W event,
    // we try to associate it with other W events,
    // since now know when it's executed.
    ConnectWithWEvents(event_info.value());

    // We also prune any redundant edges.
    PruneEdges(block_id);

    // If the current event is active, we can infer that
    // it has been previously executed. Therefore, we have
    // to associate the previous block with the first event
    // that is created inside the current one.
    if (event_info.value().HasAttribute(EXECUTED_ATTR)) {
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
  dep_graph.AddNodeAttr(block->GetBlockId(), EXECUTED_ATTR);
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
    dep_graph.AddNode(block_id, construct_default_event());
  }

  size_t event_id = new_event->GetEventId();
  dep_graph.AddNode(event_id, new_event->GetEvent());

  // There is a pending event that we need to connect with the newly-created
  // event.
  //
  // This pending event came from the same block, but it was created
  // the first time the corresponding block was executed.
  if (pending_ev) {
    dep_graph.AddEdge(pending_ev, event_id, graph::HAPPENS_BEFORE);
    pending_ev = 0;
  }
  // We add the dependency between the event corresponding to the current block
  // and the newly created event.
  //
  // The current block *creates* this event.
  dep_graph.AddEdge(block_id, event_id, graph::CREATES);
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
  dep_graph.AddEdge(source_ev, link_expr->GetTargetEvent(),
                    graph::HAPPENS_BEFORE);
}


void
DependencyInferenceAnalyzer::AnalyzeTrigger(Trigger *trigger_expr) {
  if (!trigger_expr) {
    return;
  }
  current_context = trigger_expr->GetEventId();
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

void DependencyInferenceAnalyzer::ProceedSEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  dep_graph.AddEdge(old_event.node_id, new_event.node_id,
                    graph::HAPPENS_BEFORE);
}


void DependencyInferenceAnalyzer::ProceedMEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  switch (new_event.node_obj.GetEventType()) {
    case Event::S:
      // The current event has higher priority (S > M).
      dep_graph.AddEdge(new_event.node_id, old_event.node_id,
                        graph::HAPPENS_BEFORE);
      break;
    case Event::M:
      if (new_event.node_obj.GetEventValue() < old_event.node_obj.GetEventValue()) {
        // The current event has higher priority because its
        // event value is smaller. (W i > W j if i < j).
        dep_graph.AddEdge(new_event.node_id, old_event.node_id,
                          graph::HAPPENS_BEFORE);
      } else {
        dep_graph.AddEdge(old_event.node_id, new_event.node_id,
                          graph::HAPPENS_BEFORE);
      }
      break;
    case Event::W:
    case Event::EXT:
      // The current event has smaller priority (W < M).
      dep_graph.AddEdge(old_event.node_id, new_event.node_id,
                        graph::HAPPENS_BEFORE);
  }
}


void DependencyInferenceAnalyzer::ProceedWEvent(EventInfo &new_event,
                                                EventInfo &old_event) {
  switch (new_event.node_obj.GetEventType()) {
    case Event::S:
    case Event::M:
      dep_graph.AddEdge(new_event.node_id, old_event.node_id,
                        graph::HAPPENS_BEFORE);
      break;
    case Event::EXT:
      // The new event is external.
      // Therefore, already created W events have higher priority than
      // the external events.
      //
      // XXX: Revisit
      if (old_event.node_obj.GetEventValue() != 0) {
        dep_graph.AddEdge(old_event.node_id, new_event.node_id,
                          graph::HAPPENS_BEFORE);
      }
    default:
      break;
  }
}


void DependencyInferenceAnalyzer::ProceedEXTEvent(EventInfo &new_event,
                                                  EventInfo &old_event) {
  switch (new_event.node_obj.GetEventType()) {
    case Event::S:
    case Event::M:
      // The current event has higher priority (S > W and M > W).
      dep_graph.AddEdge(new_event.node_id, old_event.node_id,
                        graph::HAPPENS_BEFORE);
    default:
      break;
  }
}


void DependencyInferenceAnalyzer::AddDependencies(size_t event_id, Event event) {
  for (auto alive_ev_id : alive_events) {
    optional<EventInfo> event_info_opt = dep_graph.GetNodeInfo(alive_ev_id);
    // We ignore events that are not present to the dependency graph.
    // events identical to the current block,
    // as well as inactive events.
    if (!event_info_opt.has_value() ||
        current_block->GetBlockId() == event_info_opt.value().node_id) {
      continue;
    }
    EventInfo event_info = event_info_opt.value();
    EventInfo new_event_info = EventInfo(event_id, event);
    switch (event_info.node_obj.GetEventType()) {
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
  switch (event_info.node_obj.GetEventType()) {
    case Event::S:
    case Event::M:
    case Event::EXT:
      // This event does not have a W type, so we do nothing.
      break;
    case Event::W:
      for (auto alive_ev_id : alive_events) {
        optional<EventInfo> node_info_opt = dep_graph.GetNodeInfo(alive_ev_id);
        if (!node_info_opt.has_value()) {
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
        EventInfo ei = node_info_opt.value();
        switch (ei.node_obj.GetEventType()) {
          case Event::S:
          case Event::M:
          case Event::EXT:
            break;
          case Event::W:
            if (ei.node_obj.GetEventValue() == event_info.node_obj.GetEventValue()) {
              dep_graph.AddEdge(event_info.node_id, ei.node_id,
                                graph::HAPPENS_BEFORE);
            }
          break;
        }
     }
  }
}


void DependencyInferenceAnalyzer::PruneEdges(size_t event_id) {
  optional<EventInfo> event_info_opt = dep_graph.GetNodeInfo(event_id);
  if (!event_info_opt.has_value()) {
    return;
  }
  EventInfo event_info = event_info_opt.value();
  for (auto next : event_info.before) {
    optional<EventInfo> nextev_info_opt = dep_graph.GetNodeInfo(next);
    if (!nextev_info_opt.has_value()) {
      continue;
    }

    EventInfo nextev_info = nextev_info_opt.value();
    // Iterate over the previous nodes of the the given event.
    //
    // If prev is included in the set of previous nodes of next,
    // we remove the corresponding edge, because `prev` is connected with
    // `next` via the given event.
    for (auto prev : event_info.after) {
      if (nextev_info.after.find(prev) != nextev_info.after.end()) {
        dep_graph.RemoveEdge(prev, next, graph::HAPPENS_BEFORE);
      }
    }
  }
}


void DependencyInferenceAnalyzer::DumpOutput(writer::OutWriter *out) const {
  if (!out) {
    return;
  }
  dep_graph.PrintGraph(graph_format, out->OutStream());
  // Clear out object. Cannot be reused.
  delete out;
  out = nullptr;
}


}
