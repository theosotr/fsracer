#ifndef STREAM_TRACE_GENERATOR_H
#define STREAM_TRACE_GENERATOR_H


#include <string>

#include <FStrace.h>


namespace fstrace {

inline int ParseDirFd(const std::string token) {
  if (token == "AT_FDCWD") {
    return AT_FDCWD;
    
  }
  if (!utils::IsNumber(token)) {
    return -1;
  }
  return std::stoi(token);
}


Consumes *EmitConsumes(const std::vector<std::string> &tokens);
Produces *EmitProduces(const std::vector<std::string> &tokens);
NewTask *EmitNewTask(const std::vector<std::string> &tokens);
DependsOn *EmitDependsOn(const std::vector<std::string> &tokens);
SysOp *EmitSysOp(const std::vector<std::string> &tokens);
ExecTask *EmitExecTask(const std::vector<std::string> &tokens);

// Emit Operation Expressions
NewFd *EmitNewFd(const std::vector<std::string> &tokens, size_t pid);
DelFd *EmitDelFd(const std::vector<std::string> &tokens, size_t pid);
DupFd *EmitDupFd(const std::vector<std::string> &tokens, size_t pid);
Hpath *EmitHpath(const std::vector<std::string> &tokens, size_t pid,
                 bool hpathsym);
Link *EmitLinkOrRename(const std::vector<std::string> &tokens, size_t pid,
                       bool is_link);
NewProc *EmitNewProc(const std::vector<std::string> &tokens, size_t pid);
Rename *EmitRename(const std::vector<std::string> &tokens, size_t pid);
SetCwd *EmitSetCwd(const std::vector<std::string> &tokens, size_t pid);
SetCwdFd *EmitSetCwdFd(const std::vector<std::string> &tokens, size_t pid);
Symlink *EmitSymlink(const std::vector<std::string> &tokens, size_t pid);


class StreamTraceGenerator {
public:
  StreamTraceGenerator(std::string trace_file_):
    trace_file(trace_file_),
    fp(nullptr),
    trace_line(nullptr),
    trace_line_len(0),
    has_next(false),
    in_sysop(false) {  }

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
  bool in_sysop;

  TraceNode *ParseLine(const std::string &line);
  TraceNode *ParseOperation(const std::vector<std::string> &tokens);
  TraceNode *ParseExpression(const std::vector<std::string> &tokens);
};


} // namespace fstrace

#endif
