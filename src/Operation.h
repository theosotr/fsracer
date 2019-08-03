#ifndef OPERATION_H
#define OPERATION_H

#include <iostream>

#define AT_FDCWD 0
#define FAILED (isFailed() ? " !failed" : "")


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
    virtual void Accept(analyzer::Analyzer *analyzer);
    virtual string ToString();
    virtual string GetOpName();

    void MarkFailed() {
      failed = true;
    }

    bool isFailed() {
      return failed;
    }

  private:
    bool failed;
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
      return GetOpName() + " " + to_string(fd) + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer);

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
      return GetOpName() + " "  + to_string(old_fd) + " " +
        to_string(new_fd) + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer);

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

    size_t GetDirFd() {
      return dirfd;
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
      string str = DirfdToString(dirfd);
      return GetOpName() + " " + str + " " + path +
        Hpath::EffToString(effect_type) + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer);

    static string EffToString(enum EffectType effect);

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

    void Accept(analyzer::Analyzer *analyzer);

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
    ~Link() {  };

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
      string old_dirfd_str = DirfdToString(old_dirfd);
      string new_dirfd_str = DirfdToString(new_dirfd);
      return GetOpName() + " " + old_dirfd_str + " " +
        old_path + " " + new_dirfd_str + " " + new_path + FAILED;
    };

    string GetOpName() {
      return "link";
    }

    void Accept(analyzer::Analyzer *analyzer);

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

    string GetPath() {
      return path;
    }

    size_t GetDirFd() {
      return dirfd;
    }

    int GetFd() {
      return fd;
    }

    string GetOpName() {
      return "newFd";
    }

    string ToString() {
      string dirfd_str = DirfdToString(dirfd);
      return GetOpName() + " " + dirfd_str + " " + path +
        " " + to_string(fd) + FAILED;
    };

    void Accept(analyzer::Analyzer *analyzer);

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
          return GetOpName() + " FD " + to_string(pid) + FAILED;
        case SHARE_FS:
          return GetOpName() + " FS " + to_string(pid) + FAILED;
        case SHARE_BOTH:
          return GetOpName() + " FS|FD " + to_string(pid) + FAILED;
        default:
          return GetOpName() + " " + to_string(pid) + FAILED;
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

    void Accept(analyzer::Analyzer *analyzer);
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
      return GetOpName() + " " + cwd + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer);
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

    size_t GetDirFd() {
      return dirfd;
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
      return GetOpName() + " " + to_string(dirfd) + " " + path + " " +
        target + FAILED;
    }

    void Accept(analyzer::Analyzer *analyzer);

  private:
    size_t dirfd;
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

    void Accept(analyzer::Analyzer *analyzer);

};


}

#endif
