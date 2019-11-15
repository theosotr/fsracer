#ifndef FS_FAULT_DETECTOR_H
#define FS_FAULT_DETECTOR_H

#include <filesystem>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <vector>

#include "DependencyInferenceAnalyzer.h"
#include "FaultDetector.h"
#include "FSAnalyzer.h"


namespace fs = std::filesystem;


namespace detector {


/**
 * A class that detects data races in file system resources.
 */
class FSFaultDetector : public FaultDetector {
public:
  // Some type aliases.
  using dep_graph_t = analyzer::DependencyInferenceAnalyzer::dep_graph_t;
  using file_info_t = analyzer::FSAnalyzer::file_info_t;
  using fs_access_t = analyzer::FSAnalyzer::FSAccess;
  using DepGNodeInfo = analyzer::DependencyInferenceAnalyzer::DepGNodeInfo;

  struct OrderingViolation {
    /// The file path that the two blocks are conflicting.
    std::string p;
    /// The effect that the first block has on the path.
    fs_access_t fs_access1;
    /// The effect that the second block has on the path.
    fs_access_t fs_access2;

    /** Constructor. */
    OrderingViolation(std::string p_, fs_access_t fs_access1_,
              fs_access_t fs_access2_):
      p(p_),
      fs_access1(fs_access1_),
      fs_access2(fs_access2_) {  }

    /**
     * Overriding the operator '<' so that we can insert instances of
     * this struct into a set.
     */
    friend bool operator<(const OrderingViolation &lhs,
                          const OrderingViolation &rhs) {
      return lhs.p < rhs.p;
    }

    /** String representation of an instance of this struct. */
    std::string ToString() const;
  };

  struct FSFault {
    std::vector<std::pair<fs::path, fs_access_t>> missing_inputs;
    std::vector<std::pair<fs::path, fs_access_t>> missing_outputs;
    std::map<std::string, std::set<OrderingViolation>> ordering_violations;

    void AddMissingInput(fs::path p, fs_access_t fs_access);
    void AddMissingOutput(fs::path p, fs_access_t fs_access);
    void AddOrderingViolation(fs::path, fs_access_t fs_acc1,
                              fs_access_t fs_acc2);
    std::string MISToString() const;
    std::string MOSToString() const;
    std::string OVToString() const;
  };


  using faults_t = std::unordered_map<std::string, FSFault>;

  /**
   * Constructor of the `RaceDetector` class.
   *
   * The arguments are const references of the outputs of
   * the analyzers utilized by this fault detector.
   */
  FSFaultDetector(file_info_t file_info_,
                 dep_graph_t dep_graph_,
                 std::string working_dir_,
                 bool ignore_dirs_ov_,
                 std::optional<std::string> ignore_files_conf);

  /** Gets the pretty name of this fault detector. */
  std::string GetName() const {
    return "FSFaultDetector";
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
  /// File info.
  file_info_t file_info;
  /// The dependency graph of events.
  dep_graph_t dep_graph;
  /// This the directory where we collected traces from.
  std::string working_dir;

  /// Table that tracks debug information of each event.
  /// Useful for fault reporting.
  mutable table::Table<std::string, fstrace::DebugInfo> event_info;

  mutable std::unordered_map<std::string, std::set<std::string>> cache_dfs;

  bool ignore_dirs_ov;
  std::set<std::string> filter_ov;
  std::set<std::string> filter_mis;
  std::set<std::string> filter_mos;

  void LoadFilters(const std::string &filter_file);

  /**
   * Gets the list of faults by exploiting the dependency graph
   * and the table of file accesses per block.
   */
  faults_t GetFaults() const;

  void DetectMissingInOrOut(faults_t &faults, fs::path p,
                            const std::vector<fs_access_t> &accesses,
                            bool is_output) const;
  void DetectOrderingViolation(faults_t &faults, fs::path p,
                               const std::vector<fs_access_t> &accesses) const;
  void DetectMissingInput(faults_t &faults, fs::path p,
                          const std::vector<fs_access_t> &accesses) const;
  void DetectMissingOutput(faults_t &faults, fs::path p,
                           const std::vector<fs_access_t> &accesses) const;
  
  /** Dumps reported faults to the standard output. */
  void DumpFaults(const faults_t &faults) const;

  bool HappensBefore(std::string source, std::string target) const;

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
