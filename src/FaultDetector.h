#ifndef FAULT_DETECTOR_H
#define FAULT_DETECTOR_H

#include <string>
#include <map>


namespace detector {

/**
 * An abstract class that defines the interface of an
 * fault detector.
 */
class FaultDetector {

public:
  /** Gets the name of the fault detector. */
  virtual std::string GetName() = 0;

  /**
   * Whether the fault detector supports online fault detection,
   * i.e., whether the fault detector is able to locate faults while
   * the application is running.
   */
  virtual bool SupportOnlineAnalysis() = 0;

  /** Detecting faults offline, i.e., after the execution of application. */
  virtual void Detect() const = 0;
  
  /** Detecting faults online, i.e., while the application is running. */
  virtual void Detect(std::map<std::string, void*> gen_store) const = 0;
};


} // namespace detector


#endif
