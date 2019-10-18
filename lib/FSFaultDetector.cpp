#include <experimental/filesystem>
#include <map>
#include <unordered_map>
#include <string>

#include "Debug.h"
#include "FSFaultDetector.h"
#include "Utils.h"
#include "FStrace.h"


namespace fs = experimental::filesystem;


namespace detector {


void FSFaultDetector::Detect() {
  analysis_time.Start();
  // First, get the detected faults. 
  auto faults = GetFaults();
  analysis_time.Stop();
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
      return !fstrace::Hpath::Consumes(acc2.access_type) &&
        !fstrace::Hpath::Touched(acc2.access_type);
    case fstrace::Hpath::PRODUCED:
    case fstrace::Hpath::EXPUNGED:
      return !fstrace::Hpath::Touched(acc2.access_type);
    case fstrace::Hpath::TOUCHED:
      return false;
  }
}


bool FSFaultDetector::HappensBefore(std::string source,
                                    std::string target) const {
  auto cache_it = cache_dfs.find(source);
  std::optional<DepGNodeInfo> source_info =
    dep_graph.GetNodeInfo(source);
  std::optional<DepGNodeInfo> target_info =
    dep_graph.GetNodeInfo(target);

  if (source == "main" || target == "main") {
    return true;
  }

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


void FSFaultDetector::DetectMissingInput(
    faults_t &faults,
    fs::path p,
    const std::vector<analyzer::FSAnalyzer::FSAccess>& accesses) const {
  bool found = false;
  for (auto const &acc : accesses) {
    if (!fstrace::Hpath::Consumes(acc.access_type)) {
      continue;
    }
    std::optional<DepGNodeInfo> node_info = dep_graph.GetNodeInfo(acc.task_name); 
    if (!node_info.has_value()) {
      continue;
    }
    for (auto const &dep : node_info.value().dependents) {
      if (dep.second == graph::CONSUMES) {
        if (utils::StartsWith(p.string(), dep.first)) {
          found = true;
          break;
        }
      }
    }
    if (!found && acc.task_name != "main" &&
        utils::StartsWith(p.native(), working_dir)) {
      auto it = faults.find(acc.task_name);
      if (it == faults.end()) {
        auto fault = FSFault();
        fault.AddMissingInput(p, acc);
        faults[acc.task_name] = fault;
      } else {
        it->second.AddMissingInput(p, acc);
      } 
    }
    found = false;
  }
}


void FSFaultDetector::DetectOrderingViolation(
    faults_t &faults,
    fs::path p,
    const std::vector<analyzer::FSAnalyzer::FSAccess>& accesses) const {
}


FSFaultDetector::faults_t FSFaultDetector::GetFaults() const {
  faults_t faults;
  fs_accesses_table_t::table_t table = fs_accesses.GetTable();
  for (auto it = table.begin(); it != table.end(); it++) {
    auto fs_access = *it;
    fs::path p = fs_access.first;
    DetectMissingInput(faults, p, fs_access.second);
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
        faults_t::iterator it = faults.find(first_access.task_name);
        if (it == faults.end()) {
          auto fault_desc = FSFault();
          fault_desc.AddOrderingViolation(p.native(), first_access,
                                          second_access);
          faults[first_access.task_name] = fault_desc;
        } else {
          it->second.AddOrderingViolation(p.native(), first_access,
                                          second_access);
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
  size_t ov_count = 0;
  size_t mis_count = 0;
  size_t mos_count = 0;
  std::string msg = "";
  for (auto const &fault_entry : faults) {
    auto task_faults = fault_entry.second;
    ov_count += task_faults.ordering_violations.size();
    mis_count += task_faults.missing_inputs.size();
    mos_count += task_faults.missing_outputs.size();
  }
  debug::msg() << "Ordering Violations: " <<  ov_count;
  debug::msg() << "Missing Inputs: " << mis_count;
  debug::msg() << "Missing Outputs: " << mos_count << "\n";
  debug::msg() << "Faults per Task:";
  for (auto const &fault_entry : faults) {
    auto task_name = fault_entry.first;
    debug::msg(debug::colors::YELLOW) << "* [Task: " + task_name + "]:";
    if (!fault_entry.second.missing_inputs.empty()) {
      debug::msg(debug::colors::RED) << "  Missing Inputs:";
      debug::msg(debug::colors::RED) << "  ---------------";
      debug::msg() << fault_entry.second.MISToString();
    }
    if (!fault_entry.second.missing_outputs.empty()) {
      debug::msg(debug::colors::RED) << "  Missing Inputs:";
      debug::msg(debug::colors::RED) << "  ---------------";
      debug::msg() << fault_entry.second.MOSToString();
    }
    if (!fault_entry.second.ordering_violations.empty()) {
      debug::msg(debug::colors::RED) << "  Ordering Violations:";
      debug::msg(debug::colors::RED) << "  --------------------";
      debug::msg() << fault_entry.second.OVToString();
    }
    debug::msg();
  }
}


void FSFaultDetector::FSFault::AddMissingInput(
    fs::path p, FSFaultDetector::fs_access_t fs_access) {
  missing_inputs.push_back({ p, fs_access });
}


void FSFaultDetector::FSFault::AddMissingOutput(
    fs::path p, FSFaultDetector::fs_access_t fs_access) {
  missing_outputs.push_back({ p, fs_access });
}


void FSFaultDetector::FSFault::AddOrderingViolation(
    fs::path p, FSFaultDetector::fs_access_t fs_acc1,
    FSFaultDetector::fs_access_t fs_acc2) {
  std::string task = fs_acc2.task_name;
  auto it = ordering_violations.find(task);
  if (it == ordering_violations.end()) {
    std::set<OrderingViolation> ov_set;
    ov_set.insert(OrderingViolation(p, fs_acc1, fs_acc2));
    ordering_violations[task] = ov_set;
  } else {
    it->second.insert(OrderingViolation(p, fs_acc1, fs_acc2));
  }
}


std::string FSFaultDetector::FSFault::MISToString() const {
  std::string str = "";
  for (auto const &acc : missing_inputs) {
    str += "    ==> " + acc.first.string();
    str += " (" + fstrace::Hpath::AccToString(acc.second.access_type);
    str += " by operation \"" + acc.second.operation_name + "\")\n";
  }
  return str;
}


std::string FSFaultDetector::FSFault::MOSToString() const {
  std::string str = "";
  for (auto const &acc : missing_outputs) {
    str += "    ==> " + acc.first.string();
    str += " (" + fstrace::Hpath::AccToString(acc.second.access_type);
    str += " by operation \"" + acc.second.operation_name + "\")\n";
  }
  return str;
}


std::string FSFaultDetector::FSFault::OVToString() const {
  std::string str = "";
  for (auto const &elem : ordering_violations) {
    str += "    ==> Task: " + elem.first + "\n";
    for (auto const &acc : elem.second) {
      str += "      - " + acc.p + " (" +
        fstrace::Hpath::AccToString(acc.fs_access1.access_type) +
        " at operation \"" + acc.fs_access1.operation_name + "\" and " +
        fstrace::Hpath::AccToString(acc.fs_access2.access_type) +
        " at operation \"" + acc.fs_access2.operation_name + "\")\n";
    }
  }
  return str;
}


} // namespace detector
