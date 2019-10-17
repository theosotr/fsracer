#include "FaultDetector.h"


namespace detector {

double FaultDetector::GetAnalysisTime() const {
  return analysis_time.GetTimeMillis();
}

} // namespace detector
