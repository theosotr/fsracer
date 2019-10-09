Conversion Rules
================

* A number means the X argument of a system call
* `ret = system call's return value`

```
access          -> hpath "AT_FDCWD" 0 "consumed";
chdir           -> setcwd 0;
chmod           -> hpath "AT_FDCWD" 0 "consumed";
chown           -> hpath "AT_FDCWD" 0 "consumed";
clone           -> newproc action ret;
    action:
            if 'CLONE_FS' in 1 and 'CLONE_FILES' in 1: "fdfs"
            elif 'CLONE_FS' in 1: "fs"
            elif 'CLONE_FILES' in 1: "fd"
            else: "none"
close           -> delfd 0;
dup             -> dupfd 0 ret;
dup2            -> dupfd 0 1;
dup3            -> dupfd 0 1;
execve          -> hpath "AT_FDCWD" 0 "consumed";
fchdir          -> fchdir 0;
fchmodat        -> hpath 0 1 "consumed";
fchownat        -> hpath 0 1 "consumed";
fcntl           -> if 'F_DUPFD' in 0: dupfd 0 ret; else skip;
fork            -> newproc "none" ret;
getxattr        -> hpath "AT_FDCWD" 0 "consumed";
getcwd          -> setcwd 0;
lchown          -> hpathsym 0 "consumed";
lgetxattr       -> hpathsym 0 "consumed";
lremovexattr    -> hpathsym 0 "consumed";
lsetxattr       -> hpathsym 0 "consumed";
lstat           -> hpathsym 0 "consumed";
link            -> link "AT_FDCWD" 0 "AT_FDCWD" 1;
                   hpath "AT_FDCWD" 0 "consumed";
                   hpath "AT_FDCWD" 1 "produced";
linkat          -> link 0 1 2 3;
                   hpath 0 1 "consumed";
                   hpath 2 3 "produced";
mkdir           -> hpath "AT_FDCWD" 0 "produced";
mkdirat         -> hpath 0 1 "produced";
mknob           -> hpath "AT_FDCWD" 0 "produced";
open            -> newfd "AT_FDCWD" 0 ret;
                   hpath "AT_FDCWD" 0 access;
    access: if 'O_CREAT' in 1 or 'O_TRUNC' in 1: "produced" else "consumed";
openat          -> newfd 0 1 ret;
                   hpath 0 1 access;
    access: if 'O_CREAT' in 1 or 'O_TRUNC' in 1: "produced" else "consumed";
readlink        -> hpathsym "AT_FDCWD" 0 "consumed";
readlinkat      -> hpathsym 0 1 "consumed";
removexattr     -> hpath "AT_FDCWD" 0 "consumed";
rename          -> rename "AT_FDCWD" 0 "AT_FDCWD" 1;
                -> hpath "AT_FDCWD" 0 "expunged";
                -> hpath "AT_FDCWD" 1 "produced";
renameat        -> rename 0 1 2 3;
                -> hpath 0 1 "expunged";
                -> hpath 2 3 "produced";
rmdir           -> hpathsym "AT_FDCWD" 0 "expunged";
stat            -> hpath "AT_FDCWD" 0 "consumed";
statfs          -> hpath "AT_FDCWD" 0 "consumed";
symlink         -> symlink "AT_FDCWD" 1 0;
                -> hpath "AT_FDCWD" 1 "produced";
symlinkat       -> symlink 1 2 0;
                -> hpath 1 2 "produced";
unlink          -> hpathsym "AT_FDCWD" 0 "expunged";
unlinkat        -> hpathsym 0 1 "expunged";
utime           -> hpathsym "AT_FDCWD" 0 "consumed";
utimensat       -> hpathsym 0 1 "consumed";
utimes          -> hpathsym "AT_FDCWD" 0 "consumed";
```
