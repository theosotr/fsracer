#ifndef TRACE_GENERATOR_H
#define TRACE_GENERATOR_H

#include <optional>
#include <string>

#include "Utils.h"
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
  /** Polymorphic Destructor. */
  virtual ~TraceGenerator() {  };

  /** Gets the name of the trace generator. */
  virtual std::string GetName() const = 0; 

  /** Start generating trace. */
  virtual void Start() = 0;

  /** Stop generating trace. */
  virtual void Stop() = 0;

  /** Checks whether trace collection has failed. */
  bool HasFailed() const;

  /**
   * Gets the error associated with the trace collection.
   *
   * It should be invoked only when `HasFailed()` returns true.
   */
  utils::err::Error GetErr() const;

  /** Adds a new error related to the current trace collection. */
  void AddError(utils::err::ErrType err_type, std::string errmsg,
                std::string location);

  /** Getter for the `trace_f` field. */
  trace::Trace *GetTrace() const {
    return trace_f;
  }

protected:
  /// Trace to generate.
  trace::Trace *trace_f;

  /// A field that indicates whether there was an error associated with
  /// this trace collection.
  optional<utils::err::Error> error;

};


} // namespace trace_generator

#endif
