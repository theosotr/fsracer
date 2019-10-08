ABNF
====

```
T           =   *operation *block
operation   =   "Operation" id "do" CRLF (opexp CRLF)* "Done"
opexp       =   delfd / dupfd / fchdir / hpath / hpathsym / link / newfd
opexp       =/  newproc / rename / setcwd / symlink
delfd       =   "delfd" fd
dupfd       =   "dupfd" fd fd
fchdir      =   "fchdir" fd
hpath       =   "hpath" dirfd filepath access
hpathsym    =   "hpathsym" dirfd filepath access
link        =   "link" dirfd filepath dirfd filepath
newfd       =   "newfd" dirfd filepath ret
newproc     =   "newproc" action ret
rename      =   "dirfd" filepath dirfd filepath
setcwd      =   "setcwd" filepath
symlink     =   "symlink" dirfd filepath filepath
filepath    =   [ "/" ] ; TODO
dirfd       =   AT_FDCWD / fd
access      =   "consumed" / "produced" / "expunged"
action      =   "fd" / "fs" / "fdfs" / "none"
fd          =   *DIGIT
ret         =   *DIGIT  ; return value, TODO can contain anything else?
id          =   *DIGIT *ALPHA
```
