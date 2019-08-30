#include "TraceGenerator.h"
#include "Utils.h"


namespace trace_generator {


bool TraceGenerator::HasFailed() const {
  return error.has_value();
}


utils::err::Error TraceGenerator::GetErr() const {
  return error.value();
}


void TraceGenerator::AddError(utils::err::ErrType err_type, std::string errmsg,
                              std::string location) {
  error = utils::err::Error(err_type, errmsg, location);
}


} // namespace trace_generator
