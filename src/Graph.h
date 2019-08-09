#ifndef GRAPH_H
#define GRAPH_H

#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>
#include <utility>


using namespace std;


namespace graph {

/**
 * A template class that represents a node in
 * the graph.
 *
 * Each node is presented by an ID (a positive inter) and
 * and object associateted with the given template parameter.
 */
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

  /**
   * Constructs a new node with the specified ID.
   * This node is described by the given `node_obj`.
   */
  Node(size_t node_id_, T node_obj_):
    node_id(node_id_),
    node_obj(node_obj_)
  {  }

  /** Checks whether this node has the given attribute. */
  bool HasAttribute(string attr) const {
    return attributes.find(attr) != attributes.end();
  }

  /** Adds a new attribute to the current node. */
  void AddAttribute(string attr) {
    attributes.insert(attr);
  }

  /** Removes the given attribute from the current node. */
  void RemoveAttribute(string attr) {
    attributes.erase(attr);
  }

  /** Operator overloading so that we can insert Nodes to a set. */
  friend bool operator<(const Node &lhs, const Node &rhs) {
    return lhs.node_id < rhs.node_id;
  }

};

/**
 * This class provides the default implementations used for dumping
 * nodes and edges in either CSV or DOT format.
 */
struct GraphPrinterDefault {
  public:
    /** Prints the given node in DOT format. */
    template<typename T>
    static string PrintNodeDot(size_t node_id, const T &node_obj) {
      return to_string(node_id);
    }

    /** Prints the given node in CSV format. */
    template<typename T>
    static string PrintNodeCSV(size_t node_id, const T &node_obj) {
      return to_string(node_id);
    }

    /** Prints the given edge label. */
    template<typename L>
    static string PrintEdgeLabel(L label) {
      return "";
    }

    /** Prints the given edge in CSV format. */
    template<typename T>
    static string PrintEdgeCSV(const T &source, const T &target) {
      return to_string(source.node_id) + "," + to_string(target.node_id);
    }

    /** Prints the given edge in DOT format. */
    template<typename T>
    static string PrintEdgeDot(const T &source, const T &target) {
      return to_string(source.node_id) + "->" + to_string(target.node_id);
    }
};


/**
 * This template class can be specialized to customize how the nodes
 * and edges of a graph are printed.
 *
 * It inherits from the `GraphPrinterDefault` class, so if a certain
 * specialized class does not provide an implementation for a particular
 * member, we provide the default implementation taken from
 * `GraphPrinterDefault`.
 */
template<typename T, typename L>
struct GraphPrinter : public GraphPrinterDefault {
};


/** Supported formats of the graph. */
enum GraphFormat {
  /// DOT: The graph is represented as a graphviz graph.
  DOT,
  /**
   * CSV: The dependency graph is represented as csv file that
   * contain its edge list.
   */
  CSV
};


/**
 * Template class used to represent a graph.
 *
 * This template is parameterized with the type `T` used to
 * describe a node, and the type L that represents the edge
 * labels of the graph.
 */
template<typename T, typename L>
class Graph {
  public:
    /**
     * Instantiate the type for representing node information
     * the parameters of the current template class.
     */
    using NodeInfo = Node<T, L>;
    /// Representation of the underlying graph.
    typedef unordered_map<size_t, NodeInfo> graph_t;

    /** Adds a new node to the graph. */
    void AddNode(size_t node_id, T node_obj) {
      NodeInfo node_info = NodeInfo(node_id, node_obj);
      graph.emplace(node_id, node_info);

    }

    /** Adds a new attribute to the given node. */
    void AddNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        it->second.AddAttribute(attr);
      }
    }

    /** Checks whether the given node has the given attribute. */
    bool HasNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        return it->second.HasAttribute(attr);
      }
      return false;
    }

    /** Remove the specified attribute from the given node. */
    void RemoveNodeAttr(size_t node_id, string attr) {
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        it->second.RemoveAttribute(attr);
      }
    }

    /** Gets the information associated with the given node. */
    optional<NodeInfo> GetNodeInfo(size_t node_id) {
      optional<NodeInfo> node_info;
      typename graph_t::iterator it = graph.find(node_id);
      if (it != graph.end()) {
        return it->second;
      }
      return node_info;
    }

    /**
     * Adds a new edge to the graph.
     *
     * The edge is described by the source node, the target node, and
     * a label.
     */
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

    /** Remove an edge from the graph. */
    void RemoveEdge(size_t source, size_t target, L label) {
      typename graph_t::iterator it = graph.find(source);
      if (it != graph.end()) {
        it->second.dependents.erase({ target, label });
      }
    }

    /**
     * Print the current graph using the given output stream,
     * and in the specified format.
     */
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
    /// Instantiate a new graph printer using the parameters of the template.
    using GPrinter = GraphPrinter<T, L>;
    /// Underlying graph.
    graph_t graph;
    /// Obj used to print the nodes and edges of the graph. */
    GPrinter printer;

    /** Prints the current graph in DOT format. */
    void PrintDot(ostream &os) {
      os << "digraph {\n";
      for (auto const &entry : graph) {
        NodeInfo node_info = entry.second;
        string node_str = printer.PrintNodeDot(node_info.node_id, node_info);
        // if node str is empty, then we omit printing this node.
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
          // if this edge string is empty, we omit printing this edge.
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

    /** Prints the current graph in CSV format. */
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
        // If edge string is empty, we omit printing this edge.
        if (edge_str != "") {
          os << edge_str << "," << printer.PrintEdgeLabel(get<2>(edge))
            << endl;
        }
      }
    }
};


}


#endif
