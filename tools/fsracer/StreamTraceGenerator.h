#ifndef STREAM_TRACE_GENERATOR_H
#define STREAM_TRACE_GENERATOR_H


#include <string>

#include <FStrace.h>


namespace fstrace {

Consumes *EmitConsumes(const std::string &line);
Produces *EmitProduces(const std::string &line);
NewTask *EmitNewTask(const std::string &line);
DependsOn *EmitDependsOn(const std::string &line);


class StreamTraceGenerator {
public:
  StreamTraceGenerator(std::string trace_file_):
    trace_file(trace_file_),
    fp(nullptr),
    trace_line(nullptr),
    trace_line_len(0),
    has_next(false) {  }

  void Start();

  TraceNode *GetNextTrace();

  bool HasNext() const;

  void Stop();

private:
  std::string trace_file;
  FILE *fp;
  char *trace_line;
  size_t trace_line_len;
  bool has_next;

  TraceNode *ParseLine(const std::string &line);
};


} // namespace fstrace

#endif
