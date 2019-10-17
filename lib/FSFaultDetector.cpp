#include <experimental/filesystem>
#include <map>
#include <unordered_map>
#include <string>

#include "Debug.h"
#include "FSFaultDetector.h"
#include "Utils.h"
#include "FStrace.h"


namespace fs = experimental::filesystem;
using DepGNodeInfo = analyzer::DependencyInferenceAnalyzer::DepGNodeInfo;


namespace detector {


void FSFaultDetector::Detect() {
  // First, get the detected faults.
  auto faults = GetFaults();
  // Second, report the detected faults to the standard output.
  DumpFaults(faults);
}


void FSFaultDetector::Detect(std::map<std::string, void*> gen_store) {
  // Online analysis is not supported at the moment.
};


bool FSFaultDetector::HasConflict(const fs_access_t &acc1,
                                  const fs_access_t &acc2) {
  switch (acc1.access_type) {
    case fstrace::Hpath::CONSUMED:
      return !fstrace::Hpath::Consumes(acc2.access_type);
    case fstrace::Hpath::PRODUCED:
    case fstrace::Hpath::EXPUNGED:
      return true;
  }
}


bool FSFaultDetector::HappensBefore(std::string source,
                                    std::string target) const {
  auto cache_it = cache_dfs.find(source);
  std::optional<DepGNodeInfo> source_info =
    dep_graph.GetNodeInfo(source);
  std::optional<DepGNodeInfo> target_info =
    dep_graph.GetNodeInfo(target);

  if (!source_info.has_value() || !target_info.has_value()) {
    return false;
  }

  if (!source_info.value().node_obj.IsTask() ||
      !target_info.value().node_obj.IsTask()) {
    return true;
  }

  if (cache_it == cache_dfs.end()) {
    std::set<std::string> visited = dep_graph.DFS(source); 
    cache_dfs[source] = visited;
    return visited.find(target) != visited.end(); 
  } else {
    return cache_it->second.find(target) != cache_it->second.end(); 
  }
}


FSFaultDetector::faults_t FSFaultDetector::GetFaults() const {
  faults_t faults;
  fs_accesses_table_t::table_t table = fs_accesses.GetTable();
  for (auto it = table.begin(); it != table.end(); it++) {
    auto fs_access = *it;
    fs::path p = fs_access.first;
    // Get all combinations of file accesses.
    auto acc_combs = utils::Get2Combinations(fs_access.second); 
    for (auto const &access : acc_combs) {
      auto first_access = access.first;
      auto second_access = access.second;
      event_info.AddEntry(first_access.task_name, first_access.debug_info);
      event_info.AddEntry(second_access.task_name, second_access.debug_info);
      if (first_access.task_name == second_access.task_name) {
        // The fist and the second access refer to the same block,
        // so we omit them.
        continue;
      }
      if (!HasConflict(first_access, second_access)) {
        // There is not any conflict between those file accesses,
        // so we proceed to the next iteration.
        continue;
      }

      // Check whether there is a dependency between the two blocks
      // corresponding to those file accesses with regards to the
      // dependency graph.
      bool has_dep = HappensBefore(
          first_access.task_name, second_access.task_name) ||
        HappensBefore(second_access.task_name, first_access.task_name);

      if (!has_dep) {
        // There is not any dependency, so we've just found a fault.
        auto pair_element = std::make_pair(
            first_access.task_name, second_access.task_name);
        faults_t::iterator it = faults.find(pair_element);
        FaultDesc fault_desc = FaultDesc(
            p.native(), first_access, second_access);
        if (it == faults.end()) {
          std::set<FaultDesc> fault_descs;
          fault_descs.insert(fault_desc);
          faults[pair_element] = fault_descs;
        } else {
          it->second.insert(fault_desc);
        }
      }
    }
  } 
  return faults;
}


void FSFaultDetector::DumpFaults(const faults_t &faults) const {
  if (faults.empty()) {
    return;
  }
  debug::msg() << "Detected Data Races";
  debug::msg() << "-------------------";
  debug::msg() << "Number of data races: " << faults.size();
  for (auto const &fault_entry : faults) {
    auto block_pair = fault_entry.first;
    std::optional<fstrace::DebugInfo> debug_info1 = event_info.GetValue(
        block_pair.first);
    std::optional<fstrace::DebugInfo> debug_info2 = event_info.GetValue(
        block_pair.second);
    string debug1 = "";
    string debug2 = "";
    if (debug_info1.has_value()) {
      string debug_str = debug_info1.value().ToString();
      debug1 = debug_str != "" ? debug_str : "empty";
    } else {
      debug1 = " !main";
    }
    if (debug_info2.has_value()) {
      string debug_str = debug_info2.value().ToString();
      debug2 = debug_str != "" ? debug_str : "empty";
    } else {
      debug2 = " !main";
    }
    debug::msg() << "* Event: "
      << block_pair.first << " (tags:" << debug1 << ") and Event: "
      << block_pair.second << " (tags:" << debug2 << "):";
    for (auto const &fault_desc : fault_entry.second) {
      debug::msg() << "  - " << fault_desc.ToString();
    }
  }
}


std::string FSFaultDetector::FaultDesc::ToString() const {
  return  "Path " + p + ":\n"
    + "    " + fstrace::Hpath::AccToString(fs_access1.access_type)
    + " by the first event "
    + "(operation: " + fs_access1.operation_name + ")\n"
    + "    "+ fstrace::Hpath::AccToString(fs_access2.access_type)
    + " by the second event (operation: " + fs_access2.operation_name + ")";
}


} // namespace detector
