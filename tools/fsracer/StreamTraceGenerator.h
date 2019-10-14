#ifndef STREAM_TRACE_GENERATOR_H
#define STREAM_TRACE_GENERATOR_H

#include <unordered_map>
#include <string>

#include <FStrace.h>
#include <TraceGenerator.h>


namespace trace_generator {


inline int ParseDirFd(const std::string token) {
  if (token == "AT_FDCWD") {
    return AT_FDCWD;
    
  }
  if (!utils::IsNumber(token)) {
    return -1;
  }
  return std::stoi(token);
}


class StreamTraceGenerator : public trace_generator::TraceGenerator {
public:
  StreamTraceGenerator(std::string trace_file_):
    trace_file(trace_file_),
    fp(nullptr),
    trace_line(nullptr),
    trace_line_len(0),
    has_next(false),
    in_sysop(false) {  }

  std::string GetName() const;
  void Start();
  fstrace::TraceNode *GetNextTrace();
  bool HasNext() const;
  void Stop();

private:
  std::string trace_file;
  FILE *fp;
  char *trace_line;
  size_t trace_line_len;
  bool has_next;
  bool in_sysop;
  size_t loc_line;
  std::string location;

  fstrace::TraceNode *ParseLine(const std::string &line);
  fstrace::TraceNode *ParseOperation(const std::vector<std::string> &tokens);
  fstrace::TraceNode *ParseExpression(const std::vector<std::string> &tokens);

  // Parser expressions
  fstrace::Consumes *EmitConsumes(const std::vector<std::string> &tokens);
  fstrace::Produces *EmitProduces(const std::vector<std::string> &tokens);
  fstrace::NewTask *EmitNewTask(const std::vector<std::string> &tokens);
  fstrace::DependsOn *EmitDependsOn(const std::vector<std::string> &tokens);
  fstrace::SysOp *EmitSysOp(const std::vector<std::string> &tokens);
  fstrace::ExecTask *EmitExecTask(const std::vector<std::string> &tokens);

  // Parse syscall operations
  fstrace::NewFd *EmitNewFd(const std::vector<std::string> &tokens, size_t pid);
  fstrace::DelFd *EmitDelFd(const std::vector<std::string> &tokens, size_t pid);
  fstrace::DupFd *EmitDupFd(const std::vector<std::string> &tokens, size_t pid);
  fstrace::Hpath *EmitHpath(const std::vector<std::string> &tokens, size_t pid,
                   bool hpathsym);
  fstrace::Link *EmitLinkOrRename(const std::vector<std::string> &tokens,
                                  size_t pid, bool is_link);
  fstrace::NewProc *EmitNewProc(const std::vector<std::string> &tokens,
                                size_t pid);
  fstrace::Rename *EmitRename(const std::vector<std::string> &tokens,
                              size_t pid);
  fstrace::SetCwd *EmitSetCwd(const std::vector<std::string> &tokens,
                              size_t pid);
  fstrace::SetCwdFd *EmitSetCwdFd(const std::vector<std::string> &tokens,
                                  size_t pid);
  fstrace::Symlink *EmitSymlink(const std::vector<std::string> &tokens,
                                size_t pid);
};


} // namespace trace_generator

#endif
