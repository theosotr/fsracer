#!/usr/bin/env python3
import argparse
import os
import re
import sys
from collections import namedtuple
from collections import deque
from functools import wraps
from abc import ABC,abstractmethod


################################# STRUCTURES ##################################

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

MATCHES = {
    '[': ']', '(': ')', '{': '}', '"': '"'
}

# A Trace is composed of process id, system call name, system call arguments,
# and system call return value.
# pid,name,ret are strings, args is a list of strings
Trace = namedtuple(
    'Trace', 'pid syscall_name syscall_args syscall_ret'
)

############################## HELPER FUNCTIONS  ##############################

def print_warning(string):
    """Print a colored warning message"""
    print(bcolors.WARNING + string + bcolors.ENDC, file=sys.stderr)


def remove_ext(string):
    """Remove the extension from a file path.
    """
    index = string.rfind('.')
    if index != -1:
        string = string[:index]
    return string

def safe_split(string):
    """Split a string using commas as delimiter safely.

    Args:
        string

    Returns:
        list

    Examples:
     1)
        input: 'AT_FDCWD, "s1.c, s2.c", O_RDONLY|O_NOCTTY'
        returns: ['AT_FDCWD', '"s1.c, s2.c"', 'O_RDONLY|O_NOCTTY']
     2)
        input: 'AT_FDCWD, "(s1.c, s2.c), s3.c", O_RDONLY|O_NOCTTY'
        returns: ['AT_FDCWD', '"(s1.c, s2.c), s3.c"', 'O_RDONLY|O_NOCTTY']
    """
    index = 0
    inside = None
    res = []
    for counter, c in enumerate(string):
        if c in ('[', '(', '{', '"') and not inside:
            inside = c
            continue
        if inside and c == MATCHES[inside]:
            inside = None
            continue
        if c == ',' and not inside:
            res.append(string[index:counter].strip())
            index = counter + 1
    res.append(string[index:len(string)].strip())  # Add last element
    return res

############################## PARSING FUNCTIONS ##############################

def split_line(line):
    """Split an fsracer line into pid and trace.
    We use the following regex to capture two groups: pid and trace.
    ^([0-9]+)[ ]+(.*)
    Args:
        line: string (fsracer output line)
    Returns:
        (pid, trace)
    Example:
        input: "12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: ('12498', 'openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3')
    """
    return re.search("^([0-9]+)[ ]+(.*)", line).groups()


def parse_unfinished(trace):
    """Parse unfinished trace.
    We use the following regex to find unfinished traces
    ^([a-z0-9_]+)\((.*)[ ]+<unfinished ...>
    If it matches return system call name, and system call arguments.
    Otherwise, return None.
    Args:
        trace: string
    Returns:
        (syscall_name, syscall_args) or None
    Example:
     1)
        input: "openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>"
        returns: ('rt_sigprocmask', 'SIG_SETMASK, [],')
    """
    syscall_re = r'^{}\({}[ ]+<unfinished ...>'.format(
        "([a-z0-9_]+)", "(.*)"
    )
    try:
        return re.search(syscall_re, trace).groups()
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None


def parse_resumed(trace):
    """Parse resumed trace.
    We use the following regex to oarse resumed traces
    ^<...[ ]+([a-z0-9_]+)[ ]+resumed>[ ]*(.*)\)[ ]*=[ ]*(-?[0-9\?]+)[ ]*.*?
    If it matches return system call name, rest of system call arguments, and
    system call return value. Otherwise, return None.
    Args:
        trace: string
    Returns:
        (syscall_name, syscall_args, syscall_ret_val) or None
    Example:
     1)
        input: "openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "<... rt_sigprocmask resumed> NULL, 8) = 0"
        returns: ['rt_sigprocmask', 'NULL 8', '0']
    3)
        input: "<... dup2 resumed> ) = 0"
        returns: ['dup2', '', '0']
    """
    syscall_re =\
        r'^<...[ ]+{}[ ]+resumed>[ ]*{}\)[ ]*=[ ]*{}[ ]*.*?'.format(
            "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
        )
    try:
        return re.search(syscall_re, trace).groups()
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None


def parse_trace(trace):
    """Parse a trace
    We use the following regex to match system call name, system call
    arguments, and system call return value. The three groups of the regex
    match syscall name, syscall args, and syscall return value.
    ^([a-z0-9_]+)\((.*)\)[ ]*=[ ]*(-?[0-9\?]+)[ ]*.*?
    Args:
        trace: String
    Returns:
        [syscall_name, syscall_args, syscall_ret_val]
    Example:
     1)
        input: "openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: ['openat', ['AT_FDCWD', '"s1.c"', 'O_RDONLY|O_NOCTTY'], '3']
     2)
        input: "access("/etc/ld.so.preload", R_OK) = -1 ENOENT (No such file or directory)"
        returns: ['access', ['"/etc/ld.so.preload"', 'R_OK'], '-1']
    """

    syscall_re = r'^{}\({}\)[ ]*=[ ]*{}[ ]*.*?'.format(
        "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
    )
    try:
        res = list(re.search(syscall_re, trace).groups())
        res[1] = safe_split(res[1])
        return res
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None


def parse_line(line, unfinished):
    """Parse strace line.

    Args:
        line: An strace line
        unfinished: A dict to save unfinished traces

    Returns:
        trace or None

    Example:
     1)
        input = {}
        input: '12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3', unfinished
        result:
            return [('12498', 'openat', ['AT_FDCWD', '"s1.c"', 'O_RDONLY|O_NOCTTY'], '3')]
            unfinished = {}
        input: '12499 rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>', unfinished
        result:
            return None
            unfinished = {('12499', 'rt_sigprocmask'): 'SIG_SETMASK, [], '}
        input: '12499 <... rt_sigprocmask resumed> NULL, 8) = 0', unfinished
        result:
            return ('12499', 'rt_sigprocmask', ['SIG_SETMASK', '[]', 'NULL', '8'], '0')
            unfinished = {}
    """
    line = line.rstrip()
    pid, trace = split_line(line)
    if any(trace.startswith(x) for x in ['---', '+++']):  # Skip Signals
        pass
    elif trace.endswith('>'):
        syscall_name, syscall_args = parse_unfinished(trace)
        # Warning: What happens if there are two unfinished system call
        # from the same pid with the same system call names?
        unfinished[(pid, syscall_name)] = syscall_args
    elif trace.startswith('<'):
        syscall_name, rest_syscall_args, syscall_ret_val = parse_resumed(trace)
        syscall_args = safe_split(
            unfinished[(pid, syscall_name)] + rest_syscall_args
        )
        del unfinished[(pid, syscall_name)]
        return Trace(pid, syscall_name, syscall_args, syscall_ret_val)
    else:
        temp_trace = parse_trace(trace)
        if temp_trace:
            return Trace(pid, temp_trace[0], temp_trace[1], temp_trace[2])

############################# TRANSLATE FUNCTIONS #############################

# def translate_<system_call_name>(trace):
#   return [operation_expression, ...]

hpath       = 'hpath {} {} {}'        # 'hpath' dirfd PATH access
hpathsym    = 'hpathsym {} {} {}'     # 'hpathsym' dirfd PATH access
setcwd      = 'setcwd {}'             # 'setcwd' PATH
newproc     = 'newproc {} {}'         # 'newproc' ['fd'/'fs'/'fdfs'/'none'] ret
delfd       = 'delfd {}'              # 'delfd' fd
dupfd       = 'dupfd {} {}'           # 'dupfd' fd fd
setcwdfd    = 'setcwdfd {}'             # 'setcwdfd' fd
symlink     = 'symlink {} {} {}'      # 'symlink' dirfd PATH PATH
link        = 'link {} {} {} {}'      # 'link' dirfd PATH dirfd PATH
rename      = 'rename {} {} {} {}'    # 'rename' dirfd PATH dirfd PATH
newfd       = 'newfd {} {} {}'        # 'newfd' dirfd PATH ret


CONSUMED = 'consumed'
EXPUNGED = 'expunged'
PRODUCED = 'produced'
TOUCHED  = 'touched'


def check_paths(paths):
    """A very specific decorator to check if any path starts with '0x'.

    If so, return None else translate the trace to the proper operation
    expression.

    To use the decorator pass as argument a list with the indexes to check from
    trace.syscall_args.
    """
    def wrap(translate_fun):
        @wraps(translate_fun)
        def wrapped_translate_fun(trace):
            if any(trace.syscall_args[x].startswith('0x') for x in paths):
                return
            return translate_fun(trace)
        return wrapped_translate_fun
    return wrap


@check_paths([0])
def translate_access(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_chdir(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [setcwd.format(trace.syscall_args[0])]


@check_paths([0])
def translate_chmod(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_chown(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


def translate_clone(trace):
    if trace.syscall_ret == '?':
        return None
    if int(trace.syscall_ret) < 0:
        return None
    has_clone_fs = 'CLONE_FS' in trace.syscall_args[1]
    has_clone_files = 'CLONE_FILES' in trace.syscall_args[1]
    if (has_clone_fs and has_clone_files):
        return [newproc.format('fdfs', trace.syscall_ret)]
    elif has_clone_fs:
        return [newproc.format('fs', trace.syscall_ret)]
    elif has_clone_files:
        return [newproc.format('fd', trace.syscall_ret)]
    else:
        return [newproc.format('none', trace.syscall_ret)]


def translate_close(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [delfd.format(trace.syscall_args[0])]


def translate_dup(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [dupfd.format(trace.syscall_args[0], trace.syscall_ret)]


def translate_dup2(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [dupfd.format(trace.syscall_args[0], trace.syscall_args[1])]


def translate_dup3(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [dupfd.format(trace.syscall_args[0], trace.syscall_args[1])]


@check_paths([0])
def translate_execve(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], CONSUMED)]


def translate_fchdir(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [setcwdfd.format(trace.syscall_args[0])]


@check_paths([1])
def translate_fchmodat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], TOUCHED
        )
    ]


@check_paths([1])
def translate_fchownat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], TOUCHED
        )
    ]


def translate_fcntl(trace):
    if int(trace.syscall_ret) < 0:
        return None
    if 'F_DUPFD' in trace.syscall_args[1]:
        return [dupfd.format(trace.syscall_args[0], trace.syscall_ret)]
    return


def translate_fork(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [newproc.format('none', trace.syscall_ret)]


@check_paths([0])
def translate_getxattr(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_getcwd(trace):
    return [setcwd.format(trace.syscall_args[0])]


@check_paths([0])
def translate_lchown(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_lgetxattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_lremovexattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_lsetxattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_lstat(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0, 1])
def translate_link(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        link.format(
            'AT_FDCWD', trace.syscall_args[0],'AT_FDCWD', trace.syscall_args[1]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[0], CONSUMED),
        hpath.format('AT_FDCWD', trace.syscall_args[1], PRODUCED)
    ]


@check_paths([1, 3])
def translate_linkat(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        link.format(
            trace.syscall_args[0], trace.syscall_args[1],
            trace.syscall_args[2], trace.syscall_args[3]
        ),
        hpath.format(trace.syscall_args[0], trace.syscall_args[1], CONSUMED),
        hpath.format(trace.syscall_args[2], trace.syscall_args[3], PRODUCED)
    ]


@check_paths([0])
def translate_mkdir(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], PRODUCED)]


@check_paths([1])
def translate_mkdirat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], PRODUCED
        )
    ]


@check_paths([0])
def translate_mknod(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], PRODUCED)]


@check_paths([0])
def translate_open(trace):
    if int(trace.syscall_ret) < 0:
        return None
    if any(x in trace.syscall_args[1] for x in ['O_CREAT', 'O_TRUNC']):
        access = PRODUCED
    else:
        access = CONSUMED
    return [
        newfd.format('AT_FDCWD', trace.syscall_args[0], trace.syscall_ret),
        hpath.format('AT_FDCWD', trace.syscall_args[0], access)
    ]


@check_paths([1])
def translate_openat(trace):
    if int(trace.syscall_ret) < 0:
        return None
    if any(x in trace.syscall_args[2] for x in ['O_CREAT', 'O_TRUNC']):
        access = PRODUCED
    else:
        access = CONSUMED
    return [
        newfd.format(trace.syscall_args[0], trace.syscall_args[1],
                     trace.syscall_ret),
        hpath.format(trace.syscall_args[0], trace.syscall_args[1], access)
    ]


@check_paths([0])
def translate_readlink(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], CONSUMED)]


@check_paths([1])
def translate_readlinkat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], CONSUMED
        )
    ]


@check_paths([0])
def translate_removexattr(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0, 1])
def translate_rename(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        rename.format(
            'AT_FDCWD', trace.syscall_args[0], 'AT_FDCWD', trace.syscall_args[1]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[0], EXPUNGED),
        hpath.format('AT_FDCWD', trace.syscall_args[1], PRODUCED)
    ]



@check_paths([1, 3])
def translate_renameat(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        rename.format(
            trace.syscall_args[0], trace.syscall_args[1],
            trace.syscall_args[2], trace.syscall_args[3]
        ),
        hpath.format(trace.syscall_args[0], trace.syscall_args[1], EXPUNGED),
        hpath.format(trace.syscall_args[2], trace.syscall_args[3], PRODUCED)
    ]


@check_paths([0])
def translate_rmdir(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], EXPUNGED)]


@check_paths([0])
def translate_stat(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0])
def translate_statfs(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([0, 1])
def translate_symlink(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        symlink.format(
            'AT_FDCWD', trace.syscall_args[1], trace.syscall_args[0]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[1], PRODUCED)

    ]


@check_paths([2])
def translate_symlinkat(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [
        symlink.format(
            trace.syscall_args[1],
            trace.syscall_args[2],
            trace.syscall_args[0]
        ),
        hpath.format(trace.syscall_args[1], trace.syscall_args[2], PRODUCED)
    ]


@check_paths([0])
def translate_unlink(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], EXPUNGED)]


@check_paths([1])
def translate_unlinkat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], EXPUNGED
        )
    ]


@check_paths([0])
def translate_utime(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


@check_paths([1])
def translate_utimensat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], TOUCHED
        )
    ]


@check_paths([0])
def translate_utimes(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], TOUCHED)]


def translate_vfork(trace):
    if int(trace.syscall_ret) < 0:
        return None
    return [newproc.format('none', trace.syscall_ret)]


def translate_write(trace):
    return [trace.syscall_args[1]]


def translate_writev(trace):
    return [trace.syscall_args[1]]

##############################  MAIN FUNCTIONS   ##############################

# trace_entries
exec_task_begin = 'execTask {} {{'      #  task_id
exec_task_end   = '}'
new_task        = 'newTask {} {}'       #  task_id task_type
depends_on      = 'dependsOn {} {}'     #  task_id task_id (child, parent)
consumes        = 'consumes {} "{}"'    #  task_id string (prerequisite)
produces        = 'produces {} {}'      #  task_id string (Gradle)


def to_sysop(opexp, opid, tabs, return_type=''):
    ret = "{}sysop {} SYNC {{\n".format(tabs * "\t", opid)
    is_failed = ""
    if return_type.strip().startswith("-1"):
        is_failed = " !failed"
    for op in opexp:
        ret += "{}{}{}\n".format( (tabs+1) * "\t", op, is_failed)
    ret += (tabs * "\t") + "}"
    return ret


def write_out(out, output):
    out.write(output + '\n')


class Handler(ABC):

    def __init__(self, inp, out, working_dir):
        self.inp = inp
        self.out = out
        self.working_dir = working_dir
        self.unfinished = {}
        self.sys_op_id = ''  # syscall-name_line-number

    def _set_currend_directory(self):
        pid = self.trace.pid
        opexp = [pid + ", " + setcwd.format('"' + self.working_dir + '"')]
        write_out(self.out, to_sysop(opexp, 'init', 0))

    @abstractmethod
    def _handle_write(self):
        pass

    def _handle_opexp(self, opexp):
        # Add pid
        opexp = [self.trace.pid + ", " + x for x in opexp]
        write_out(
            self.out,
            to_sysop(opexp, self.sys_op_id, 0, self.trace.syscall_ret)
        )

    def execute(self):
        first_trace = False
        for counter, line in enumerate(self.inp):
            self.trace = parse_line(line, self.unfinished)
            if self.trace:
                if not first_trace:
                    first_trace = True
                    self._set_currend_directory()
                self.sys_op_id = "{}_{}".format(
                    self.trace.syscall_name, counter
                )
                opexp = globals()["translate_" + self.trace.syscall_name](self.trace)
                if any(self.trace.syscall_name == x
                       for x in ('write', 'writev')):
                    self._handle_write()
                elif opexp is not None:
                    self._handle_opexp(opexp)


class GradleHandler(Handler):
    GRADLE_PREFIX = "##GRADLE##"

    LOGGING_FD = 100

    def __init__(self, inp, out, working_dir):
        super(GradleHandler, self).__init__(inp, out, working_dir)

    def _handle_begin(self, write_str):
        _, task_name = write_str.split(
            '"{} Begin '.format(self.GRADLE_PREFIX), 1)
        write_out(self.out, exec_task_begin.format(task_name[:-1].strip()))

    def _handle_end(self):
        write_out(self.out, exec_task_end)

    def _handle_gradle(self, write_str):
        _, construct = write_str.split('"{} '.format(self.GRADLE_PREFIX))
        write_out(self.out, construct[:-1].strip().replace('\\"', '"'))

    def _handle_write(self):
        if int(self.trace.syscall_args[0]) <= self.LOGGING_FD or int(self.trace.syscall_args[0]) >= 300:
            return

        write_str = self.trace.syscall_args[1]
        if write_str.startswith('"{} Begin '.format(self.GRADLE_PREFIX)):
            self._handle_begin(write_str)
        elif write_str.startswith('"{} End '.format(self.GRADLE_PREFIX)):
            self._handle_end()
        elif write_str.startswith('"{} '.format(self.GRADLE_PREFIX)):
            self._handle_gradle(write_str)
        else:
            pass


class MakeHandler(Handler):
    def __init__(self, inp, out, working_dir):
        super(MakeHandler, self).__init__(inp, out, working_dir)
        self.nesting_counter = 0  # Count how many tabs to put
        self.cwd_queue = deque()  # Save the history of cwd
        self.cwd_queue.append('')
        self.is_open_task = False
        self.current_task_id = None
        # {'path': {'file/rule': ('task_id', [prereqs])}}
        self.depends_on = {}  # Save info to find depends on relations
        self.task_ids = set()
        # {'target_without_ext': ('target', 'task_id')}
        self.targets = {}  # We need it for lookups in find_included heuristic.
        # A structure to save info about current included file
        # {'basename': '', 'target': '', 'task_id': '', 'FD': '', 'contents': ''}
        self.current_incl = {}
        # Include files declared into the Makefile
        self.included = set()


    def _handle_write(self):
        message = self.trace.syscall_args[1].replace('"', '').replace('\\n', '').strip()
        # TODO: Add tests
        begin = re.search("^##BEGIN##: +(.*)###(.*)###(.*)", message)
        if begin:
            # target: the name of whichever target caused the rule’s recipe
            #         to be run or rule name
            makefile, target, prereq = begin.groups()
            makefile = makefile.strip()
            target = target.strip()
            prereq = prereq.strip()
            prereqs = re.split(' |,', prereq)
            # In case of nested we should have the current_path
            cwd = self.cwd_queue[-1]
            cwd = cwd + "/" if cwd != '' else cwd
            task_id = "{}{}_{}".format(cwd, makefile, target)
            tabs = self.nesting_counter * '\t'
            self.current_task_id = task_id
            if task_id not in self.task_ids:
                self.task_ids.add(task_id)
                self.targets[remove_ext(target)] = (target, task_id)
                write_out(
                    self.out,
                    tabs + new_task.format(self.current_task_id, "W 1")
                )
                self._write_consumes(prereqs)
            write_out(
                self.out,
                tabs + exec_task_begin.format(task_id)
            )
            # Add values to depends_on dict
            if cwd not in self.depends_on:
                self.depends_on[cwd] = {target: [task_id, prereqs]}
            elif target not in self.depends_on[cwd]:
                self.depends_on[cwd][target] = [task_id, prereqs]
            self.nesting_counter += 1
            return
        if message.startswith("##END##"):
            self.nesting_counter -= 1
            write_out(
                self.out,
                (self.nesting_counter * '\t') + exec_task_end
            )
            return
        entering = re.search("^.*:[ ]+Entering directory[ ]+'(.*)'", message)
        if entering:
            self.cwd_queue.append(entering.groups()[0])
            return
        leaving = re.search("^.*:[ ]+Leaving directory[ ]+'(.*)'", message)
        if leaving:
            self.cwd_queue.pop()
            return
        if (self.current_incl and
            self.trace.syscall_args[0] == self.current_incl['FD']):
            self.current_incl['contents'] += self.trace.syscall_args[1]
        return

    def _handle_opexp(self, opexp):
        # Add pid
        opexp = [self.trace.pid + ", " + x for x in opexp]
        write_out(
            self.out,
            to_sysop(
                opexp, self.sys_op_id,
                self.nesting_counter, self.trace.syscall_ret
            )
        )
        # Handle openat, open, stat, and close for include case
        if self.trace.syscall_name == 'close' and self.current_incl:
            if int(self.trace.syscall_args[0]) == int(self.current_incl['FD']):
                temp = self.current_incl['contents']
                temp = temp.replace('\\\\', '').replace('\\n', '').replace('"', '')
                contents = ""
                for path in temp.split():
                    if path[0] != '/' or path.startswith(self.working_dir):
                        contents += path + ' '
                prereqs = contents[contents.find(':')+1:].split()
                self._write_consumes(prereqs)
                # Add values to depends_on dict
                cwd = self.cwd_queue[-1]
                cwd = cwd + "/" if cwd != '' else cwd
                task_id = self.current_incl['task_id']
                target = self.current_incl['target']
                if cwd not in self.depends_on:
                    self.depends_on[cwd] = {target: [task_id, prereqs]}
                elif target not in self.depends_on[cwd]:
                    self.depends_on[cwd][target] = [task_id, prereqs]
                elif target in self.depends_on[cwd]:
                    temp_prereqs = set(self.depends_on[cwd][target][1])
                    temp_prereqs.update(prereqs)
                    self.depends_on[cwd][target][1] = list(temp_prereqs)
                self.current_incl = {}
        elif (self.trace.syscall_name in ('open', 'openat')
              and not self.current_incl):
            filename = self.trace.syscall_args[1].replace('"', '')
            basename = remove_ext(filename)
            # Maybe we should only check only for .d files
            if (basename in self.targets
                and int(self.trace.syscall_ret) > 0
                and filename in self.included
               ):
                self.current_incl['basename'] = basename
                self.current_incl['FD'] = self.trace.syscall_ret
                self.current_incl['task_id'] = self.targets[basename][1]
                self.current_incl['target'] = self.targets[basename][0]
                self.current_incl['contents'] = ""
        elif self.trace.syscall_name == 'stat':
            filename = self.trace.syscall_args[0].replace('"', '')
            if filename.endswith('.d'):
                self.included.add(filename)

    def _find_depends_on_relations(self):
        # Set a dependsOn relation if one of the prerequisites is a target in
        # another rule.
        # WARNING: what happens if a rule has multiple targets ($@ is the first)
        for path, rules in self.depends_on.items():
            for _, (task_id, prereqs) in rules.items():
                for prereq in prereqs:
                    # We only care for rules inside current Makefile
                    if prereq in self.depends_on[path]:
                        depends_on_entry = depends_on.format(
                            task_id, self.depends_on[path][prereq][0]
                        )
                        write_out(self.out, depends_on_entry)

    def _write_consumes(self, prereqs):
        # We use the current_task_id whereas in inside case it may not be
        # the case. TODO ask dds
        cwd = self.cwd_queue[-1]
        cwd = cwd + "/" if cwd != '' else cwd
        tabs = self.nesting_counter * '\t'
        for x in prereqs:
            working_dir = self.working_dir if cwd == '' else cwd
            write_out(
                self.out,
                tabs + consumes.format(
                    self.current_task_id,
                    os.path.join(working_dir, x))
            )

    def execute(self):
        super(MakeHandler, self).execute()
        self._find_depends_on_relations()
        if self.nesting_counter != 0:
            print_warning(
                "Warning: ENDs not matching BEGINs, " + str(self.nesting_counter)
            )


def main(inp, out, program, working_dir):
    if program == "make":
        handler = MakeHandler(inp, out, working_dir)
    elif program == "gradle":
        handler = GradleHandler(inp, out, working_dir)
    handler.execute()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert strace's traces to fstraces."
    )
    parser.add_argument('working_dir')
    parser.add_argument('-c', '--context', choices=['make', 'gradle'],
                        default='make'
                       )
    parser.add_argument('-i', '--input')
    # TODO
    #  parser.add_argument('-o', '--output')
    args = parser.parse_args()
    inp = sys.stdin
    out = sys.stdout
    if args.input:
        with open(args.input, 'r') as f:
            inp = f.readlines()
    main(inp, sys.stdout, args.context, args.working_dir)
