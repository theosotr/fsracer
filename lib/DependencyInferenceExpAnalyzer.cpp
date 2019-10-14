#include "DependencyInferenceExpAnalyzer.h"


namespace graph {


bool TaskDir::IsTask() const {
  return task.has_value();
}


fstrace::Task TaskDir::GetTask() const {
  return task.value();
}

} // namespace graph


namespace analyzer {

std::string DependencyInferenceAnalyzer::GetName() const {
  return "DependencyInferenceAnalyzer";
}


} // namespace analyzer
