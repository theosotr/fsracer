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
  /// Set of node attributes.
  set<string> attributes;

  Node(size_t node_id_, T node_obj_):
    node_id(node_id_),
    node_obj(node_obj_)
  {  }

  bool HasAttribute(string attr) const {
    return attributes.find(attr) != attributes.end();
  }

  void AddAttribute(string attr) {
    attributes.insert(attr);
  }

  void RemoveAttribute(string attr) {
    attributes.erase(attr);
  }

  friend bool operator<(const Node &lhs, const Node &rhs) {
    return lhs.node_id < rhs.node_id;
  }

};


struct GraphPrinterDefault {
  public:
    template<typename T>
    static string PrintNodeDot(size_t node_id, const T &node_obj) {
      return to_string(node_id);
    }

    template<typename T>
    static string PrintNodeCSV(size_t node_id, const T &node_obj) {
      return to_string(node_id);
    }

    template<typename L>
    static string PrintEdgeLabel(L label) {
      return "";
    }

    template<typename T>
    static string PrintEdgeCSV(const T &source, const T &target) {
      return to_string(source.node_id) + "," + to_string(target.node_id);
    }

    template<typename T>
    static string PrintEdgeDot(const T &source, const T &target) {
      return to_string(source.node_id) + "->" + to_string(target.node_id);
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

    void AddNode(size_t node_id, T node_obj) {
      NodeInfo node_info = NodeInfo(node_id, node_obj);
      graph.emplace(node_id, node_info);

    }

    void AddNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        it->second.AddAttribute(attr);
      }
    }

    bool HasNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        return it->second.HasAttribute(attr);
      }
      return false;
    }

    void RemoveNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        it->second.RemoveAttribute(attr);
      }
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
          optional<NodeInfo> target_info_opt = GetNodeInfo(dependent.first);
          if (!target_info_opt.has_value()) {
            continue;
          }
          NodeInfo target_info = target_info_opt.value();
          string edge_str = printer.PrintEdgeDot(node_info, target_info);
          if (edge_str == "") {
            continue;
          }

          os << edge_str << "[label=\""
            << printer.PrintEdgeLabel(dependent.second)
            << "\"];"
            << endl;
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
      set<tuple<NodeInfo, NodeInfo, L>> edges;
      for (auto const &entry : graph) {
        NodeInfo node_info = entry.second;
        for (auto const &dependent: node_info.dependents) {
          optional<NodeInfo> target_info_opt = GetNodeInfo(dependent.first);
          if (!target_info_opt.has_value()) {
            continue;
          }
          NodeInfo target_info = target_info_opt.value();
          edges.insert(make_tuple(node_info, target_info,
                                  dependent.second));
        }
      }
      for (auto const &edge : edges) {
        string edge_str = printer.PrintEdgeCSV(get<0>(edge), get<1>(edge));
        if (edge_str != "") {
          os << edge_str << "," << printer.PrintEdgeLabel(get<2>(edge))
            << endl;
        }
      }
    }
};


}


#endif
