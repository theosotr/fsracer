#ifndef GRAPH_H
#define GRAPH_H

#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>
#include <utility>


using namespace std;


namespace graph {

template<typename T, typename L>
struct Node {
  /// The ID of the current node.
  size_t node_id;
  /// Type information of the node.
  T node_obj;
  /// The set of nodes that are dependent on the current one.
  set<pair<size_t, L>> dependents;
  /// Set of nodes executed before the current one.
  set<size_t> before;
  /// Set of nodes executed after the current one.
  set<size_t> after;
  bool active;

  Node(size_t node_id_, T node_obj_):
    node_id(node_id_),
    node_obj(node_obj_),
    active(false)
  {  }

};


struct GraphPrinterDefault {
  public:
    template<typename T>
    static string PrintNodeDot(size_t node_id, T node_obj) {
      return to_string(node_id);
    }

    template<typename T>
    static string PrintNodeCSV(size_t node_id, T node_obj) {
      return to_string(node_id);
    }

    template<typename L>
    static string PrintEdgeLabel(L label) {
      return "";
    }
};


template<typename T, typename L>
struct GraphPrinter : public GraphPrinterDefault {
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


template<typename T, typename L>
class Graph {
  public:
    using NodeInfo = Node<T, L>;
    typedef unordered_map<size_t, NodeInfo> graph_t;

    void MarkActive(size_t node_id) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        it->second.active = true;
      }
    }

    void AddNode(size_t node_id, T node_obj) {
      NodeInfo node_info = NodeInfo(node_id, node_obj);
      graph.emplace(node_id, node_info);

    }

    optional<NodeInfo> GetNodeInfo(size_t node_id) {
      optional<NodeInfo> node_info;
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        return it->second;
      }
      return node_info;
    }


    void AddEdge(size_t source, size_t target, L label) {
      if (source == target) {
        return;
      }
      typename graph_t::iterator it = graph.find(source);
      if (it != graph.end()) {
        it->second.dependents.insert({ target, label });
        typename graph_t::iterator target_it = graph.find(target);
        if (target_it == graph.end()) {
          return;
        }
        it->second.before.insert(target);
        target_it->second.after.insert(source);
      }
    }

    void RemoveEdge(size_t source, size_t target, L label) {
      typename graph_t::iterator it = graph.find(source);
      if (it != graph.end()) {
        it->second.dependents.erase({ target, label });
      }
    }

    void PrintGraph(enum GraphFormat graph_format, ostream &os) {
      switch (graph_format) {
        case DOT:
          PrintDot(os);
          break;
        case CSV:
          PrintCSV(os);
      }
    }

  private:
    using GPrinter = GraphPrinter<T, L>;
    graph_t graph;
    GPrinter printer;

    void PrintDot(ostream &os) {
      os << "digraph {\n";
      for (auto const &entry : graph) {
        NodeInfo node_info = entry.second;
        string node_str = printer.PrintNodeDot(node_info.node_id, node_info);
        if (node_str != "") {
          os << node_str << ";" << endl;
        }
        for (auto const &dependent : node_info.dependents) {
          optional<NodeInfo> target_info = GetNodeInfo(dependent.first);
          if (!target_info.has_value()) {
            continue;
          }
          if (target_info.value().active) {
            os << node_info.node_id << "->"
              << dependent.first
              << "[label=\""
              << printer.PrintEdgeLabel(dependent.second)
              << "\"];"
              << endl;
          }
        }
      }
      os << "}" << endl;
    }

    void PrintCSV(ostream &os) {
      // We store the graph in uniform form,
      // the edge list is sorted by on the value of
      // each source and target.
      //
      // The order is ascending.
      set<tuple<size_t, size_t, L>> edges;
      for (auto const &entry : graph) {
        NodeInfo node_info = entry.second;
        if (!node_info.active) {
          continue;
        }
        for (auto const &dependent: node_info.dependents) {
          optional<NodeInfo> target_info = GetNodeInfo(dependent.first);
          if (!target_info.has_value()) {
            continue;
          }
          if (target_info.value().active) {
            edges.insert(make_tuple(node_info.node_id, dependent.first,
                                    dependent.second));
          }
        }
      }
      for (auto const &edge : edges) {
        os << get<0>(edge)
           << ","
           << get<1>(edge)
           << ","
           << printer.PrintEdgeLabel(get<2>(edge))
           << "\n";
      }
    }
};


}


#endif
