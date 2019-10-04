#ifndef OPERATION_H
#define OPERATION_H

#include <iostream>

#define AT_FDCWD 0
#define FAILED (failed ? " !failed" : "")
#define ACTUAL_NAME (actual_op_name != "" ? " !" + actual_op_name : "")


using namespace std;

namespace analyzer {
  class Analyzer;
}


namespace operation {


inline string DirfdToString(size_t dirfd) {
  return dirfd == AT_FDCWD ? "AT_FDCWD" : to_string(dirfd);
}


class Operation {

  public:
    Operation():
      failed(false) {  }
    virtual ~Operation() {  };
    virtual void Accept(analyzer::Analyzer *analyzer) const = 0;
    virtual string ToString() const = 0;
    virtual string GetOpName() const = 0;

    void MarkFailed() {
      failed = true;
    }

    bool isFailed() const {
      return failed;
    }

    void SetActualOpName(string actual_op_name_) {
      actual_op_name = actual_op_name_;
    }

    string GetActualOpName() const {
      return actual_op_name;
    }

  protected:
    bool failed;
    string actual_op_name;
};


class DelFd : public Operation {
  public:
    DelFd(size_t fd_):
      fd(fd_) {  }

    ~DelFd() {  }

    size_t GetFd() const {
      return fd;
    }

    string GetOpName() const {
      return "delFd";
    }

    string ToString() const {
      return GetOpName() + " " + to_string(fd) + ACTUAL_NAME + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer) const;

  private:
    size_t fd;
};


class DupFd : public Operation {
  public:
    DupFd(size_t old_fd_):
      old_fd(old_fd_),
      new_fd(0) {  }
    ~DupFd() {  }

    size_t GetOldFd() const {
      return old_fd;
    }

    size_t GetNewFd() const {
      return new_fd;
    }

    void SetNewFd(size_t fd) {
      new_fd = fd;
    }

    string GetOpName() const {
      return "dupFd";
    }

    string ToString() const {
      return GetOpName() + " "  + to_string(old_fd) + " " +
        to_string(new_fd) + ACTUAL_NAME + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer) const;

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

    Hpath(size_t dirfd_, string path_, enum EffectType effect_type_):
      dirfd(dirfd_),
      path(path_),
      effect_type(effect_type_) {  }
    ~Hpath() {  }

    size_t GetDirFd() const {
      return dirfd;
    }

    string GetPath() const {
      return path;
    }

    enum EffectType GetEffectType() const {
      return effect_type;
    }

    string GetOpName() const {
      return "hpath";
    }

    string ToString() const {
      string str = DirfdToString(dirfd);
      return GetOpName() + " " + str + " " + path + " " +
        Hpath::EffToString(effect_type) + ACTUAL_NAME + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer) const;

    static string EffToString(enum EffectType effect);

    static bool Consumes(enum EffectType effect);

  protected:
    size_t dirfd;
    string path;
    enum EffectType effect_type;

};


class HpathSym : public Hpath {
  public:
    HpathSym(size_t dirfd_, string path_, enum EffectType effect_type_):
      Hpath(dirfd_, path_, effect_type_) {  }
    ~HpathSym() {  }

    void Accept(analyzer::Analyzer *analyzer) const;

    string GetOpName() const {
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
    ~Link() {  };

    size_t GetOldDirfd() const {
      return old_dirfd;
    }

    size_t GetNewDirfd() const {
      return new_dirfd;
    }

    string GetOldPath() const {
      return old_path;
    }

    string GetNewPath() const {
      return new_path;
    }

    string ToString() const {
      string old_dirfd_str = DirfdToString(old_dirfd);
      string new_dirfd_str = DirfdToString(new_dirfd);
      return GetOpName() + " " + old_dirfd_str + " " +
        old_path + " " + new_dirfd_str + " " + new_path + ACTUAL_NAME + FAILED;
    };

    string GetOpName() const {
      return "link";
    }

    void Accept(analyzer::Analyzer *analyzer) const;

  private:
    size_t old_dirfd;
    string old_path;
    size_t new_dirfd;
    string new_path;

};



class NewFd : public Operation {
  public:
    NewFd(size_t dirfd_, string path_, int fd_):
      dirfd(dirfd_),
      path(path_),
      fd(fd_) { }

    ~NewFd() {  }

    string GetPath() const {
      return path;
    }

    size_t GetDirFd() const {
      return dirfd;
    }

    int GetFd() const {
      return fd;
    }

    string GetOpName() const {
      return "newFd";
    }

    string ToString() const {
      string dirfd_str = DirfdToString(dirfd);
      if (failed) {
        return GetOpName() + " " + dirfd_str + " " + path +
          ACTUAL_NAME + FAILED;
      }
      return GetOpName() + " " + dirfd_str + " " + path +
        " " + to_string(fd) + ACTUAL_NAME + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer) const;

  private:
    size_t dirfd;
    string path;
    int fd;
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

    enum CloneMode GetCloneMode() const {
      return clone_mode;
    }

    size_t GetPID() const {
      return pid;
    }

    void SetPID(size_t pid_) {
      pid = pid_;
    }

    string GetOpName() const  {
      return "newProc";
    }

    string ToString() const {
      switch (clone_mode) {
        case SHARE_FD:
          return GetOpName() + " FD " + to_string(pid) + ACTUAL_NAME + FAILED;
        case SHARE_FS:
          return GetOpName() + " FS " + to_string(pid) + ACTUAL_NAME + FAILED;
        case SHARE_BOTH:
          return GetOpName() + " FS|FD " + to_string(pid) + ACTUAL_NAME + FAILED;
        default:
          return GetOpName() + " " + to_string(pid) + ACTUAL_NAME + FAILED;
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
    ~Rename() {  };

    string GetOpName() const {
      return "rename";
    }

    void Accept(analyzer::Analyzer *analyzer) const;
};


class SetCwd : public Operation {
  public:
    SetCwd(string cwd_):
      cwd(cwd_) {  }
    ~SetCwd() {  }

    string GetCwd() const {
      return cwd;
    }

    string GetOpName() const {
      return "setCwd";
    }

    string ToString() const {
      return GetOpName() + " " + cwd + ACTUAL_NAME + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer) const;
  private:
    string cwd;
};


class Symlink : public Operation {
  public:
    Symlink(size_t dirfd_, string path_, string target_):
      dirfd(dirfd_),
      path(path_),
      target(target_) {  }
    ~Symlink() {  }

    size_t GetDirFd() const {
      return dirfd;
    }

    string GetPath() const {
      return path;
    }

    string GetTargetPath() const {
      return target;
    }

    string GetOpName() const {
      return "symlink";
    }

    string ToString() const {
      return GetOpName() + " " + DirfdToString(dirfd) + " " + path + " " +
        target + ACTUAL_NAME + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer) const;

  private:
    size_t dirfd;
    string path;
    string target;

};


class Nop : public Operation {
  public:
    string GetOpName() const {
      return "nop";
    }

    string ToString() const {
      return GetOpName();
    }

    void Accept(analyzer::Analyzer *analyzer) const;

};


}

#endif
