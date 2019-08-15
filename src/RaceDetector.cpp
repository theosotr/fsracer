#include <experimental/filesystem>
#include <map>
#include <string>

#include "Debug.h"
#include "Operation.h"
#include "RaceDetector.h"
#include "Utils.h"


namespace fs = experimental::filesystem;
namespace op = operation;


namespace detector {


void RaceDetector::Detect() {
  // First, get the detected faults.
  auto faults = GetFaults();
  // Second, report the detected faults to the standard output.
  DumpFaults(faults);
}

void RaceDetector::Detect(std::map<std::string, void*> gen_store) {
  // Online analysis is not supported at the moment.
};


bool RaceDetector::HasConflict(const fs_access_t &acc1,
                               const fs_access_t &acc2) {
  switch (acc1.effect_type) {
    case op::Hpath::CONSUMED:
      return !op::Hpath::Consumes(acc2.effect_type);
    case op::Hpath::PRODUCED:
    case op::Hpath::EXPUNGED:
      return true;
  }
}


RaceDetector::faults_t RaceDetector::GetFaults() const {
  faults_t faults;
  table::EffectTable::table_t table = fs_accesses.GetTable();
  for (auto it = table.begin(); it != table.end(); it++) {
    auto fs_access = *it;
    fs::path p = fs_access.first;
    // Get all combinations of file accesses.
    auto acc_combs = utils::Get2Combinations(fs_access.second); 
    for (auto const &access : acc_combs) {
      auto first_access = access.first;
      auto second_access = access.second;
      event_info.AddEntry(first_access.event_id, first_access.debug_info);
      event_info.AddEntry(second_access.event_id, second_access.debug_info);
      if (first_access.event_id == second_access.event_id) {
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
      bool has_dep = dep_graph.HasPath(
          first_access.event_id, second_access.event_id) ||
        dep_graph.HasPath(second_access.event_id, first_access.event_id);

      if (!has_dep) {
        // There is not any dependency, so we've just found a fault.
        auto pair_element = std::make_pair(
            first_access.event_id, second_access.event_id);
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


void RaceDetector::DumpFaults(const faults_t &faults) const {
  if (faults.empty()) {
    return;
  }
  for (auto const &fault_entry : faults) {
    auto block_pair = fault_entry.first;
    optional<DebugInfo> debug_info1 = event_info.GetValue(block_pair.first);
    optional<DebugInfo> debug_info2 = event_info.GetValue(block_pair.second);
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
    debug::msg() << "Detected Data Races";
    debug::msg() << "-------------------";
    debug::msg() << "Number of data races: " << faults.size();
    debug::msg() << "* Event: "
      << block_pair.first << " (tags:" << debug1 << ") and Event: "
      << block_pair.second << " (tags:" << debug2 << "):";
    for (auto const &fault_desc : fault_entry.second) {
      debug::msg() << "  - " << fault_desc.ToString();
    }
  }
}


std::string RaceDetector::FaultDesc::ToString() const {
  return  "Path " + p + ": "
    + op::Hpath::EffToString(fs_access1.effect_type)
    + " by the first block and "
    + op::Hpath::EffToString(fs_access2.effect_type)
    + " by the second one.";
}


} // namespace detector
