ABNF
====

```
T           =   *trace_entry
trace_entry =   expression / execTask
execTask    =   "execTask" task_id "{" CRLF *(expression CRLF) "}"
expression  =   newTask / dependOn / sysOp / consumes / produces
newTask     =   "newTask" task_id task_type
dependOn    =   "dependOn" task_id task_id
            ; We need it in Node and Gradle
consumes    =   "consumes" task_id string
            ; In Make and Gradle are the prerequisites
produces    =   "produces" task_id string ; We don't need it in make
task_type   =   "S" *DIGIT / "M" *DIGIT / "W" *DIGIT / "EXTERNAL"
            ; In Make it is always S and in Gradle W
sysop       =   "sysop" opId "SYNC" sysop_body
            /   "sysop" opId "ASYNC" task_id sysop_body
            ; We need it only in Node
sysop_body  =   "{" CRLF *(pid opexp CRLF) "}"
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
task_id     =   id
opId        =   id
pid         =   *DIGIT
filepath    =   [ "/" ] ; TODO
dirfd       =   AT_FDCWD / fd
access      =   "consumed" / "produced" / "expunged"
action      =   "fd" / "fs" / "fdfs" / "none"
fd          =   *DIGIT
ret         =   *DIGIT  ; return value, TODO can contain anything else?
id          =   *DIGIT *ALPHA
```
