#ifndef TRACE_GENERATOR_H
#define TRACE_GENERATOR_H

#include <string>

#include "Trace.h"


namespace trace_generator {

/**
 * This is a simple interface for generating traces.
 *
 * All classes that are responsible for generating traces (e.g.,
 * generating trace through DynamoRIO or generating trace statically by
 * parsing a trace file) should inherit from this class.
 */
class TraceGenerator {
public:
  /** Polymorphic destructor. */
  virtual ~TraceGenerator() = 0;

  /** Gets the name of the trace generator. */
  virtual std::string GetName() const = 0; 

  /** Start generating trace. */
  virtual void Start() = 0;

  /** Stop generating trace. */
  virtual void Stop() = 0;

  /** Getter for the `trace_f` field. */
  trace::Trace *GetTrace() const {
    return trace_f;
  }

private:
  /** Trace to generate. */
  trace::Trace *trace_f;

};


} // namespace trace_generator

#endif
