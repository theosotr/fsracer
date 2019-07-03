#ifndef OPERATION_H
#define OPERATION_H

#include <iostream>

#define AT_FDCWD 0


using namespace std;

namespace interpreter {
  class Interpreter;
}


namespace operation {

class Operation {

  public:
    virtual ~Operation() {  };
    virtual void Accept(interpreter::Interpreter *interpreter);
    virtual string ToString();
    virtual string GetOpName();
};


class DelFd : public Operation {
  public:
    DelFd(size_t fd_):
      fd(fd_) {  }

    ~DelFd() {  }

    size_t GetFd() {
      return fd;
    }

    string GetOpName() {
      return "delFd";
    }

    string ToString() {
      return GetOpName() + " " + to_string(fd);
    }

    void Accept(interpreter::Interpreter *interpreter);

  private:
    size_t fd;
};


class DupFd : public Operation {
  public:
    DupFd(size_t old_fd_):
      old_fd(old_fd_),
      new_fd(0) {  }
    ~DupFd() {  }

    size_t GetOldFd() {
      return old_fd;
    }

    size_t GetNewFd() {
      return new_fd;
    }

    void SetNewFd(size_t fd) {
      new_fd = fd;
    }

    string GetOpName() {
      return "dupFd";
    }

    string ToString() {
      return GetOpName() + " "  + to_string(old_fd) + " " + to_string(new_fd);
    };

    void Accept(interpreter::Interpreter *interpreter);

  private:
      size_t old_fd;
      size_t new_fd;

};


class Hpath : public Operation {
  public:
    enum EffectType {
      PRODUCED,
      CONSUMED,
      EXPUNGED
    };

    Hpath(size_t dir_fd_, string path_, enum EffectType effect_type_):
      dir_fd(dir_fd_),
      path(path_),
      effect_type(effect_type_) {  }
    ~Hpath() {  }

    size_t GetDirFd() {
      return dir_fd;
    }

    string GetPath() {
      return path;
    }

    enum EffectType GetEffectType() {
      return effect_type;
    }

    string GetOpName() {
      return "hpath";
    }

    string ToString() {
      string str = dir_fd == AT_FDCWD ? "AT_FDCWD" : to_string(dir_fd);
      switch (effect_type) {
        case PRODUCED:
          return GetOpName() + " " + str + " " + path + " produced";
        case CONSUMED:
          return GetOpName() + " " + str + " " + path + " consumed";
        default:
          return GetOpName() + " " + str + " " + path + " expunged";
      }
    };

    void Accept(interpreter::Interpreter *interpreter);

  protected:
    size_t dir_fd;
    string path;
    enum EffectType effect_type;

};


class HpathSym : public Hpath {
  public:
    HpathSym(size_t dir_fd_, string path_, enum EffectType effect_type_):
      Hpath(dir_fd_, path_, effect_type_) {  }
    ~HpathSym() {  }

    void Accept(interpreter::Interpreter *interpreter);

    string GetOpName() {
      return "hpathsym";
    }
};


class Link : public Operation {
  public:
    Link(size_t old_dirfd_, string old_path_, size_t new_dirfd_,
         string new_path_):
      old_dirfd(old_dirfd_),
      old_path(old_path_),
      new_dirfd(new_dirfd_),
      new_path(new_path_) {  }
    ~Link();

    size_t GetOldDirfd() {
      return old_dirfd;
    }

    size_t GetNewDirfd() {
      return new_dirfd;
    }

    string GetOldPath() {
      return old_path;
    }

    string GetNewPath() {
      return new_path;
    }

    string ToString() {
      return GetOpName() + " " + to_string(old_dirfd) + " " +
        old_path + " " + to_string(new_dirfd) + " " + new_path;
    };

    string GetOpName() {
      return "link";
    }

    void Accept(interpreter::Interpreter *interpreter);

  private:
    size_t old_dirfd;
    string old_path;
    size_t new_dirfd;
    string new_path;

};



class NewFd : public Operation {
  public:
    NewFd(string path_):
      path(path_),
      fd(0) { }

    ~NewFd() {  }

    string GetPath() {
      return path;
    }

    size_t GetFd() {
      return fd;
    }

    void SetFd(size_t fd_) {
      fd = fd_;
    }

    string GetOpName() {
      return "newFd";
    }

    string ToString() {
      return GetOpName() + " " + path + " " + to_string(fd);
    };

    void Accept(interpreter::Interpreter *interpreter);

  private:
    string path;
    size_t fd;
};


class NewProc : public Operation {
  public:
    enum CloneMode {
      SHARE_FS,
      SHARE_FD,
      SHARE_BOTH,
      SHARE_NONE
    };

    NewProc(enum CloneMode clone_mode_):
      clone_mode(clone_mode_),
      pid(0) {  }
    ~NewProc() {  }

    enum CloneMode GetCloneMode() {
      return clone_mode;
    }

    size_t GetPID() {
      return pid;
    }

    void SetPID(size_t pid_) {
      pid = pid_;
    }

    string GetOpName() {
      return "newProc";
    }

    string ToString() {
      switch (clone_mode) {
        case SHARE_FD:
          return GetOpName() + " FD " + to_string(pid);
        case SHARE_FS:
          return GetOpName() + " FS " + to_string(pid);
        case SHARE_BOTH:
          return GetOpName() + " FS|FD " + to_string(pid);
        default:
          return GetOpName() + " " + to_string(pid);
      }
    }

  private:
    enum CloneMode clone_mode;
    size_t pid;
};


class Rename : public Link {
  public:
    Rename(size_t old_dirfd_, string old_path_, size_t new_dirfd_,
         string new_path_):
      Link(old_dirfd_, old_path_, new_dirfd_, new_path_) {  }
    ~Rename();

    string GetOpName() {
      return "rename";
    }

    void Accept(interpreter::Interpreter *interpreter);
};


class SetCwd : public Operation {
  public:
    SetCwd(string cwd_):
      cwd(cwd_) {  }
    ~SetCwd() {  }

    string GetCwd() {
      return cwd;
    }

    string GetOpName() {
      return "setCwd";
    }

    string ToString() {
      return GetOpName() + " " + cwd;
    }

    void Accept(interpreter::Interpreter *interpreter);
  private:
    string cwd;
};


class Symlink : public Operation {
  public:
    Symlink(size_t dir_fd_, string path_, string target_):
      dir_fd(dir_fd_),
      path(path_),
      target(target_) {  }
    ~Symlink() {  }

    size_t GetDirFd() {
      return dir_fd;
    }

    string GetPath() {
      return path;
    }

    string GetTargetPath() {
      return target;
    }

    string GetOpName() {
      return "symlink";
    }

    string ToString() {
      return GetOpName() + " " + to_string(dir_fd) + " " + path + " " +
        target;
    }

    void Accept(interpreter::Interpreter *interpreter);

  private:
    size_t dir_fd;
    string path;
    string target;

};


class Nop : public Operation {
  public:
    string GetOpName() {
      return "nop";
    }

    string ToString() {
      return GetOpName();
    }

    void Accept(interpreter::Interpreter *interpreter);

};


}

#endif
