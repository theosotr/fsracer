#ifndef RACE_DETECTOR_H
#define RACE_DETECTOR_H

#include <map>
#include <unordered_map>
#include <string>

#include "DependencyInferenceAnalyzer.h"
#include "FaultDetector.h"
#include "FSAnalyzer.h"
#include "Operation.h"


namespace op = operation;


namespace detector {


/**
 * A class that detects data races in file system resources.
 */
class RaceDetector : public FaultDetector {
public:
  // Some type aliases.
  using dep_graph_t = analyzer::DependencyInferenceAnalyzer::dep_graph_t;
  using fs_accesses_table_t = analyzer::FSAnalyzer::fs_accesses_table_t;
  using fs_access_t = analyzer::FSAnalyzer::FSAccess;

  /**
   * This struct describes a fault associated with two
   * conflicting blocks.
   */
  struct FaultDesc {
    /// The file path that the two blocks are conflicting.
    std::string p;
    /// The effect that the first block has on the path.
    fs_access_t fs_access1;
    /// The effect that the second block has on the path.
    fs_access_t fs_access2;

    /** Constructor. */
    FaultDesc(std::string p_, fs_access_t fs_access1_,
              fs_access_t fs_access2_):
      p(p_),
      fs_access1(fs_access1_),
      fs_access2(fs_access2_) {  }

    /**
     * Overriding the operator '<' so that we can insert instances of
     * this struct into a set.
     */
    friend bool operator<(const FaultDesc &lhs, const FaultDesc &rhs) {
      return lhs.p < rhs.p;
    }

    /** String representation of an instance of this struct. */
    std::string ToString() const;
  };

  // FIXME: Use hash map.
  using faults_t = std::map<std::pair<size_t, size_t>, std::set<FaultDesc>>;

  /**
   * Constructor of the `RaceDetector` class.
   *
   * The arguments are const references of the outputs of
   * the analyzers utilized by this fault detector.
   */
  RaceDetector(const fs_accesses_table_t &fs_accesses_,
               const dep_graph_t &dep_graph_):
    fs_accesses(fs_accesses_),
    dep_graph(dep_graph_) {  }

  /** Gets the pretty name of this fault detector. */
  std::string GetName() const {
    return "RaceDetector";
  }

  /** Checks whether this fault detector supports online analysis. */
  bool SupportOnlineAnalysis() const {
    return false;
  }

  /** Detecting data races. */
  void Detect(void);
  /** Detecting data races as the application runs. */
  void Detect(std::map<std::string, void*> gen_store);

private:
  /// File accesses per block.
  const fs_accesses_table_t &fs_accesses;
  /// The dependency graph of events.
  const dep_graph_t &dep_graph;

  /// Table that tracks debug information of each event.
  /// Useful for fault reporting.
  mutable table::Table<size_t, trace::DebugInfo> event_info;

  mutable unordered_map<size_t, set<size_t>> cache_dfs;

  /**
   * Gets the list of faults by exploiting the dependency graph
   * and the table of file accesses per block.
   */
  faults_t GetFaults() const;
  
  /** Dumps reported faults to the standard output. */
  void DumpFaults(const faults_t &faults) const;

  bool HappensBefore(size_t source, size_t target) const;

  /**
   * Checks whether there is a conflict between the first file access
   * and the second one.
   *
   * A conflict exist when two blocks access the same file, and at least
   * one of them produces it or expunges it.
   */
  static bool HasConflict(const fs_access_t &acc1, const fs_access_t &acc2);
};


} // namespace detector

#endif
