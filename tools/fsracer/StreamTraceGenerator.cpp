#include <algorithm>
#include <cstring>
#include <regex>
#include <stdio.h>
#include <sstream>

#include "Debug.h"
#include "StreamTraceGenerator.h"
#include "Utils.h"


namespace fstrace {
// TODO Handle parser errors

Consumes *EmitConsumes(const std::vector<std::string> &tokens) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  return new Consumes(tokens[1], tokens[2]);
}


Produces *EmitProduces(const std::vector<std::string> &tokens) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  return new Produces(tokens[1], tokens[2]);
}


NewTask *EmitNewTask(const std::vector<std::string> &tokens) {
  switch (tokens.size()) {
    case 4: {
      const std::string &task_name = tokens[1];
      const std::string &task_type = tokens[2];
      if (!utils::IsNumber(tokens[3])) {
        return nullptr;
      }
      size_t task_value = std::stoi(tokens[3]);
      if (task_type == "S") {
        return new NewTask(task_name, Task(Task::S, task_value));
      } else if (task_type == "M") {
        return new NewTask(task_name, Task(Task::M, task_value));
      } else if (task_type == "W") {
        return new NewTask(task_name, Task(Task::W, task_value));
      } else {
        return nullptr;
      }
      break;
    } case 3: {
      const std::string &task_name = tokens[1];
      const std::string &task_type = tokens[2];
      if (task_type != "EXTERNAL") {
        return nullptr;
      }
      return new NewTask(task_name, Task(Task::EXT, 0));
    } default:
      return nullptr;
  }
}


DependsOn *EmitDependsOn(const std::vector<std::string> &tokens) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  return new DependsOn(tokens[1], tokens[2]);
}


SysOp *EmitSysOp(const std::vector<std::string> &tokens) {
  switch (tokens.size()) {
    case 4: {
      const std::string &op_type = tokens[2];
      if (op_type != "SYNC") {
        return nullptr;
      }
      return new SysOp(tokens[1]);
    } case 5: {
      const std::string &op_type = tokens[3];
      if (op_type != "ASYNC") {
        return nullptr;
      }
      return new SysOp(tokens[1], tokens[2]);
    } default:
      return nullptr;
  }
}


ExecTask *EmitExecTask(const std::vector<std::string> &tokens) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  return new ExecTask(tokens[1]);
}


NewFd *EmitNewFd(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 5) {
    return nullptr;
  }
  size_t dirfd = ParseDirFd(tokens[2]);
  if (dirfd < 0) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[4])) {
    return nullptr;
  }
  int fd = std::stoi(tokens[4]);
  NewFd *newfd = new NewFd(pid, dirfd, tokens[3], fd);
  if (fd < 0) {
    newfd->MarkFailed();
  }
  return newfd;
}


DelFd *EmitDelFd(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[2])) {
    return nullptr;
  }
  return new DelFd(pid, std::stoi(tokens[2]));
}


DupFd *EmitDupFd(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 4) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[2])) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[3])) {
    return nullptr;
  }
  return new DupFd(pid, std::stoi(tokens[2]), std::stoi(tokens[3]));
}


Hpath *EmitHpath(const std::vector<std::string> &tokens, size_t pid,
                 bool hpathsym) {
  if (tokens.size() != 5) {
    return nullptr;
  }
  size_t dirfd = ParseDirFd(tokens[2]);
  if (dirfd < 0) {
    return nullptr;
  }
  enum Hpath::AccessType access_type;
  if (tokens[4] == "consumed") {
    access_type = Hpath::CONSUMED;
  } else if (tokens[4] == "produced") {
    access_type = Hpath::PRODUCED;
  } else if (tokens[4] == "expunged") {
    access_type = Hpath::EXPUNGED;
  } else {
    return nullptr;
  }
  if (hpathsym) {
    return new HpathSym(pid, dirfd, tokens[3], access_type);
  }
  return new Hpath(pid, dirfd, tokens[3], access_type);

}


Link *EmitLinkOrRename(const std::vector<std::string> &tokens, size_t pid,
                       bool is_link) {
  if (tokens.size() != 6) {
    return nullptr;
  }
  size_t old_dirfd = ParseDirFd(tokens[2]);
  if (old_dirfd < 0) {
    return nullptr;
  }
  size_t new_dirfd = ParseDirFd(tokens[4]);
  if (new_dirfd < 0) {
    return nullptr;
  }
  if (is_link) {
    return new Link(pid, old_dirfd, tokens[3], new_dirfd, tokens[5]);
  }
  return new Rename(pid, old_dirfd, tokens[3], new_dirfd, tokens[5]);
}


NewProc *EmitNewProc(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 4) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[3])) {
    return nullptr;
  }
  size_t new_pid = std::stoi(tokens[3]);
  const std::string &clone_mode = tokens[2];
  if (clone_mode == "fs") {
    return new NewProc(pid, NewProc::SHARE_FS, new_pid);
  } else if (clone_mode == "fd") {
    return new NewProc(pid, NewProc::SHARE_FD, new_pid);
  } else if (clone_mode == "fdfs") {
    return new NewProc(pid, NewProc::SHARE_BOTH, new_pid);
  } else if (clone_mode == "none") {
    return new NewProc(pid, NewProc::SHARE_NONE, new_pid);
  } else {
    return nullptr;
  }
}


SetCwd *EmitSetCwd(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  return new SetCwd(pid, tokens[2]);
}


SetCwdFd *EmitSetCwdFd(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 3) {
    return nullptr;
  }
  if (!utils::IsNumber(tokens[2])) {
    return nullptr;
  }
  return new SetCwdFd(pid, std::stoi(tokens[2]));
}


Symlink *EmitSymlink(const std::vector<std::string> &tokens, size_t pid) {
  if (tokens.size() != 5) {
    return nullptr;
  }
  size_t dirfd = ParseDirFd(tokens[2]);
  if (dirfd < 0) {
    return nullptr;
  }
  return new Symlink(pid, dirfd, tokens[3], tokens[4]);
}


void StreamTraceGenerator::Start() {
  fp = fopen(trace_file.c_str(), "r");
  if (fp == nullptr) {
    std::stringstream ss;
    ss << strerror(errno);
    debug::err("TraceError") << "Error while opening file "
        + trace_file + ": " + ss.str();
    exit(EXIT_FAILURE);
  }
  has_next = true;
}


TraceNode *StreamTraceGenerator::GetNextTrace() {
  if (!has_next) {
    return nullptr;
  }
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


TraceNode *StreamTraceGenerator::ParseLine(const std::string &line) {
  std::string l = line;
  l.pop_back();
  l.erase(0, l.find_first_not_of(" \t\n\r\f\v"));
  std::vector<std::string> tokens;
  utils::Split(l, tokens);
  if (!in_sysop) {
    return ParseExpression(tokens);
  }
  return ParseOperation(tokens);
}


TraceNode *StreamTraceGenerator::ParseOperation(
    const std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
    return nullptr;
  }
  const std::string &first_tok = tokens[0];
  if (first_tok == "}") {
    in_sysop = false;
    return nullptr;
  }
  size_t pos = first_tok.find(',');
  if (pos == std::string::npos) {
    return nullptr;
  }
  std::string pid_str = first_tok.substr(0, pos);
  if (!utils::IsNumber(pid_str)) {
    return nullptr;
  }
  if (tokens.size() < 2) {
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
    return nullptr;
  }
}


TraceNode *StreamTraceGenerator::ParseExpression(
    const std::vector<std::string> &tokens) {
  if (tokens.size() < 1) {
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
    SysOp *sysop = EmitSysOp(tokens);
    if (sysop) {
      in_sysop = true;
    }
    return sysop;
  } else if (expr == "execTask") {
    return EmitExecTask(tokens);
  }
}


} // namespace fstrace
