#ifndef RACE_DETECTOR_H
#define RACE_DETECTOR_H

#include <map>
#include <string>

#include "DependencyInferenceAnalyzer.h"
#include "EffectTable.h"
#include "FaultDetector.h"
#include "Operation.h"


namespace op = operation;


namespace detector {


/**
 * A class that detects data races in file system resources.
 */
class RaceDetector : public FaultDetector {
public:
  /**
   * This struct describes a fault associated with two
   * conflicting blocks.
   */
  struct FaultDesc {
    /// The file path that the two blocks are conflicting.
    std::string p;
    /// The effect that the first block has on the path.
    enum op::Hpath::EffectType block1_eff;
    /// The effect that the second block has on the path.
    enum op::Hpath::EffectType block2_eff;

    /** Constructor. */
    FaultDesc(std::string p_, enum op::Hpath::EffectType block1_eff_,
              enum op::Hpath::EffectType block2_eff_):
      p(p_),
      block1_eff(block1_eff_),
      block2_eff(block2_eff_) {  }

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

  // Some type aliases.
  using dep_graph_t = analyzer::DependencyInferenceAnalyzer::dep_graph_t;
  using fs_access_t = std::pair<size_t, enum op::Hpath::EffectType>;
  // FIXME: Use hash map.
  using faults_t = std::map<std::pair<size_t, size_t>, std::set<FaultDesc>>;

  /**
   * Constructor of the `RaceDetector` class.
   *
   * The arguments are const references of the outputs of
   * the analyzers utilized by this fault detector.
   */
  RaceDetector(const table::EffectTable &fs_accesses_,
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
  const table::EffectTable &fs_accesses;
  /// The dependency graph of events.
  const dep_graph_t &dep_graph;

  /**
   * Gets the list of faults by exploiting the dependency graph
   * and the table of file accesses per block.
   */
  faults_t GetFaults() const;
  
  /** Dumps reported faults to the standard output. */
  void DumpFaults(const faults_t &faults) const;

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
