#include <algorithm>
#include <cstring>
#include <regex>
#include <stdio.h>
#include <sstream>

#include "Debug.h"
#include "StreamTraceGenerator.h"
#include "Utils.h"


#define CHECK_TOKENS(n, con)                                               \
  if (tokens.size() != n) {                                                \
    AddError(utils::err::TRACE_ERROR, "#con expects #n tokens", location); \
    has_next = false;                                                      \
    return nullptr;                                                        \
  }

#define CHECK_DIRFD(n)                                                    \
  if (n < 0) {                                                            \
    AddError(utils::err::TRACE_ERROR,                                     \
             "#n should be either an integer or \"AT_FDCWD\"", location); \
    has_next = false;                                                     \
    return nullptr;                                                       \
  }


namespace trace_generator {


fstrace::Consumes *
StreamTraceGenerator::EmitConsumes(const std::vector<std::string> &tokens) {
  CHECK_TOKENS(3, "consumes");
  return new fstrace::Consumes(tokens[1], tokens[2]);
}


fstrace::Produces *
StreamTraceGenerator::EmitProduces(const std::vector<std::string> &tokens) {
  CHECK_TOKENS(3, "produces");
  return new fstrace::Produces(tokens[1], tokens[2]);
}


fstrace::NewTask *
StreamTraceGenerator::EmitNewTask(const std::vector<std::string> &tokens) {
  switch (tokens.size()) {
    case 4: {
      const std::string &task_name = tokens[1];
      const std::string &task_type = tokens[2];
      if (!utils::IsNumber(tokens[3])) {
        AddError(utils::err::TRACE_ERROR,
                 "newTask expects 4th token to be a number", location);
        has_next = false;
        return nullptr;
      }
      size_t task_value = std::stoi(tokens[3]);
      if (task_type == "S") {
        return new fstrace::NewTask(
            task_name, fstrace::Task(fstrace::Task::S, task_value));
      } else if (task_type == "M") {
        return new fstrace::NewTask(
            task_name, fstrace::Task(fstrace::Task::M, task_value));
      } else if (task_type == "W") {
        return new fstrace::NewTask(
            task_name, fstrace::Task(fstrace::Task::W, task_value));
      } else {
        AddError(utils::err::TRACE_ERROR, "Unknown event type", location);
        has_next = false;
        return nullptr;
      }
      break;
    } case 3: {
      const std::string &task_name = tokens[1];
      const std::string &task_type = tokens[2];
      if (task_type != "EXTERNAL") {
        AddError(utils::err::TRACE_ERROR, "Unknown event type", location);
        has_next = false;
        return nullptr;
      }
      return new fstrace::NewTask(task_name,
                                  fstrace::Task(fstrace::Task::EXT, 0));
    } default:
      AddError(utils::err::TRACE_ERROR, "newTask expects 4 or 5 tokens",
               location);
      has_next = false;
      return nullptr;
  }
}


fstrace::DependsOn *
StreamTraceGenerator::EmitDependsOn(const std::vector<std::string> &tokens) {
  CHECK_TOKENS(3, "dependsOn")
  return new fstrace::DependsOn(tokens[1], tokens[2]);
}


fstrace::SysOpBeg *
StreamTraceGenerator::EmitSysOp(const std::vector<std::string> &tokens) {
  switch (tokens.size()) {
    case 4: {
      const std::string &op_type = tokens[2];
      if (op_type != "SYNC") {
        AddError(utils::err::TRACE_ERROR, "sysop with 4 tokens should be SYNC",
                 location);
        has_next = false;
        return nullptr;
      }
      return new fstrace::SysOpBeg(tokens[1]);
    } case 5: {
      const std::string &op_type = tokens[3];
      if (op_type != "ASYNC") {
        AddError(utils::err::TRACE_ERROR, "sysop with 5 tokens should be ASYNC",
                 location);
        has_next = false;
        return nullptr;
      }
      return new fstrace::SysOpBeg(tokens[1], tokens[2]);
    } default:
      AddError(utils::err::TRACE_ERROR, "sysop expects 4 or 5 tokens",
               location);
      has_next = false;
      return nullptr;
  }
}


fstrace::ExecTaskBeg *
StreamTraceGenerator::EmitExecTask(const std::vector<std::string> &tokens) {
  CHECK_TOKENS(3, "execTask");
  return new fstrace::ExecTaskBeg(tokens[1]);
}


fstrace::NewFd *
StreamTraceGenerator::EmitNewFd(const std::vector<std::string> &tokens,
                                size_t pid) {
  CHECK_TOKENS(5, "newfd");
  size_t dirfd = ParseDirFd(tokens[2]);
  CHECK_DIRFD(dirfd);
  if (!utils::IsNumber(tokens[4])) {
    AddError(utils::err::TRACE_ERROR, "fd should be an integer", location);
    has_next = false;
    return nullptr;
  }
  int fd = std::stoi(tokens[4]);
  fstrace::NewFd *newfd = new fstrace::NewFd(pid, dirfd, tokens[3], fd);
  if (fd < 0) {
    newfd->MarkFailed();
  }
  return newfd;
}


fstrace::DelFd *
StreamTraceGenerator::EmitDelFd(const std::vector<std::string> &tokens,
                                size_t pid) {
  CHECK_TOKENS(3, "delfd");
  if (!utils::IsNumber(tokens[2])) {
    AddError(utils::err::TRACE_ERROR, "fd should be an integer", location);
    has_next = false;
    return nullptr;
  }
  return new fstrace::DelFd(pid, std::stoi(tokens[2]));
}


fstrace::DupFd *
StreamTraceGenerator::EmitDupFd(const std::vector<std::string> &tokens,
                                size_t pid) {
  CHECK_TOKENS(4, "dupfd");
  if (!utils::IsNumber(tokens[2])) {
    AddError(utils::err::TRACE_ERROR, "old_fd should be an integer", location);
    has_next = false;
    return nullptr;
  }
  if (!utils::IsNumber(tokens[3])) {
    AddError(utils::err::TRACE_ERROR, "new_fd should be an integer", location);
    has_next = false;
    return nullptr;
  }
  return new fstrace::DupFd(pid, std::stoi(tokens[2]), std::stoi(tokens[3]));
}


fstrace::Hpath *
StreamTraceGenerator::EmitHpath(const std::vector<std::string> &tokens,
                                size_t pid, bool hpathsym) {
  CHECK_TOKENS(5, "hpath");
  size_t dirfd = ParseDirFd(tokens[2]);
  CHECK_DIRFD(dirfd);
  enum fstrace::Hpath::AccessType access_type;
  if (tokens[4] == "consumed") {
    access_type = fstrace::Hpath::CONSUMED;
  } else if (tokens[4] == "produced") {
    access_type = fstrace::Hpath::PRODUCED;
  } else if (tokens[4] == "expunged") {
    access_type = fstrace::Hpath::EXPUNGED;
  } else {
    AddError(utils::err::TRACE_ERROR, "Unknown access type", location);
    has_next = false;
    return nullptr;
  }
  if (hpathsym) {
    return new fstrace::HpathSym(pid, dirfd, tokens[3], access_type);
  }
  return new fstrace::Hpath(pid, dirfd, tokens[3], access_type);

}


fstrace::Link *
StreamTraceGenerator::EmitLinkOrRename(const std::vector<std::string> &tokens,
                                       size_t pid, bool is_link) {

  CHECK_TOKENS(6, is_link ? "link" : "rename");
  size_t old_dirfd = ParseDirFd(tokens[2]);
  CHECK_DIRFD(old_dirfd);
  size_t new_dirfd = ParseDirFd(tokens[4]);
  CHECK_DIRFD(new_dirfd);
  if (is_link) {
    return new fstrace::Link(pid, old_dirfd, tokens[3], new_dirfd, tokens[5]);
  }
  return new fstrace::Rename(pid, old_dirfd, tokens[3], new_dirfd, tokens[5]);
}


fstrace::NewProc *
StreamTraceGenerator::EmitNewProc(const std::vector<std::string> &tokens,
                                  size_t pid) {
  CHECK_TOKENS(4, "newproc");
  if (!utils::IsNumber(tokens[3])) {
    AddError(utils::err::TRACE_ERROR, "pid should be an integer", location);
    has_next = false;
    return nullptr;
  }
  size_t new_pid = std::stoi(tokens[3]);
  const std::string &clone_mode = tokens[2];
  if (clone_mode == "fs") {
    return new fstrace::NewProc(pid, fstrace::NewProc::SHARE_FS, new_pid);
  } else if (clone_mode == "fd") {
    return new fstrace::NewProc(pid, fstrace::NewProc::SHARE_FD, new_pid);
  } else if (clone_mode == "fdfs") {
    return new fstrace::NewProc(pid, fstrace::NewProc::SHARE_BOTH, new_pid);
  } else if (clone_mode == "none") {
    return new fstrace::NewProc(pid, fstrace::NewProc::SHARE_NONE, new_pid);
  } else {
    AddError(utils::err::TRACE_ERROR, "Unknown clone mode", location);
    has_next = false;
    return nullptr;
  }
}


fstrace::SetCwd *
StreamTraceGenerator::EmitSetCwd(const std::vector<std::string> &tokens,
                                 size_t pid) {
  CHECK_TOKENS(3, "setcwd");
  return new fstrace::SetCwd(pid, tokens[2]);
}


fstrace::SetCwdFd *
StreamTraceGenerator::EmitSetCwdFd(const std::vector<std::string> &tokens,
                                   size_t pid) {
  CHECK_TOKENS(3, "setcwdfd");
  if (!utils::IsNumber(tokens[2])) {
    AddError(utils::err::TRACE_ERROR, "fd should be an integer", location);
    has_next = false;
    return nullptr;
  }
  return new fstrace::SetCwdFd(pid, std::stoi(tokens[2]));
}


fstrace::Symlink *
StreamTraceGenerator::EmitSymlink(const std::vector<std::string> &tokens,
                                  size_t pid) {
  CHECK_TOKENS(5, "symlink");
  size_t dirfd = ParseDirFd(tokens[2]);
  CHECK_DIRFD(dirfd);
  return new fstrace::Symlink(pid, dirfd, tokens[3], tokens[4]);
}


void StreamTraceGenerator::Start() {
  fp = fopen(trace_file.c_str(), "r");
  if (fp == nullptr) {
    std::stringstream ss;
    ss << strerror(errno);
    AddError(utils::err::TRACE_ERROR, "Error while opening file "
        + trace_file + ": " + ss.str(), "");
    return;
  }
  has_next = true;
}


fstrace::TraceNode *StreamTraceGenerator::GetNextTrace() {
  if (!has_next) {
    return nullptr;
  }
  loc_line++;
  if (getline(&trace_line, &trace_line_len, fp) != -1) {
    return ParseLine(trace_line);
  } else {
    has_next = false;
    return nullptr;
  } 
}


void StreamTraceGenerator::Stop() {
  if (fp != nullptr) {
    fclose(fp);
  }

  if (trace_line) {
    free(trace_line);
  }
}


bool StreamTraceGenerator::HasNext() const {
  return has_next;
}


fstrace::TraceNode *StreamTraceGenerator::ParseLine(const std::string &line) {
  std::string l = line;
  l.pop_back();
  l.erase(0, l.find_first_not_of(" \t\n\r\f\v"));
  location = l + ":" + std::to_string(loc_line);
  if (l == "}") {
    in_sysop = false;
    return new fstrace::End();
  }
  std::vector<std::string> tokens;
  utils::Split(l, tokens);
  if (!in_sysop) {
    return ParseExpression(tokens);
  }
  return ParseOperation(tokens);
}


fstrace::TraceNode *StreamTraceGenerator::ParseOperation(
    const std::vector<std::string> &tokens) {
  if (tokens.size() < 2) {
    AddError(utils::err::TRACE_ERROR,
             "Tokens of a sysop operations should be at least 2", location);
    has_next = false;
    return nullptr;
  }
  const std::string &first_tok = tokens[0];
  size_t pos = first_tok.find(',');
  if (pos == std::string::npos) {
    AddError(utils::err::TRACE_ERROR, "Cannot extract pid of sysop operation",
             location);
    has_next = false;
    return nullptr;
  }
  // Get the pid of a sysop operation.
  std::string pid_str = first_tok.substr(0, pos);
  if (!utils::IsNumber(pid_str)) {
    AddError(utils::err::TRACE_ERROR, "pid should be an integer", location);
    has_next = false;
    return nullptr;
  }
  const std::string op_expr = tokens[1];
  size_t pid = std::stoi(pid_str);
  if (op_expr == "newfd") {
    return EmitNewFd(tokens, pid);
  } else if (op_expr == "delfd") {
    return EmitDelFd(tokens, pid);
  } else if (op_expr == "dupfd") {
    return EmitDupFd(tokens, pid);
  } else if (op_expr == "hpath") {
    return EmitHpath(tokens, pid, false);
  } else if (op_expr == "hpathsym") {
    return EmitHpath(tokens, pid, true);
  } else if (op_expr == "link") {
    return EmitLinkOrRename(tokens, pid, true);
  } else if (op_expr == "newproc") {
    return EmitNewProc(tokens, pid);
  } else if (op_expr == "rename") {
    return EmitLinkOrRename(tokens, pid, false);
  } else if (op_expr == "setcwd") {
    return EmitSetCwd(tokens, pid);    
  } else if (op_expr == "setcwdfd") {
    return EmitSetCwdFd(tokens, pid);
  } else if (op_expr == "symlink") {
    return EmitSymlink(tokens, pid);
  } else {
    AddError(utils::err::TRACE_ERROR, "Uknown sysop operation", location);
    return nullptr;
  }
}


fstrace::TraceNode *StreamTraceGenerator::ParseExpression(
    const std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    AddError(utils::err::TRACE_ERROR,
             "Tokens of an expression should be at least 1", location);
    has_next = false;
    return nullptr;
  }
  const std::string &expr = tokens[0];
  if (expr == "consumes") {
    return EmitConsumes(tokens);
  } else if (expr == "produces") {
    return EmitProduces(tokens);
  } else if (expr == "newTask") {
    return EmitNewTask(tokens);
  } else if (expr == "dependsOn") {
    return EmitDependsOn(tokens);
  } else if (expr == "sysop") {
    fstrace::SysOpBeg *sysop = EmitSysOp(tokens);
    if (sysop) {
      in_sysop = true;
    }
    return sysop;
  } else if (expr == "execTask") {
    return EmitExecTask(tokens);
  } else {
    has_next = false;
    AddError(utils::err::TRACE_ERROR, "Unknown expression", location);
    return nullptr;
  }
}


std::string StreamTraceGenerator::GetName() const {
  return "StreamTraceGenerator";
}

} // namespace trace_generator
