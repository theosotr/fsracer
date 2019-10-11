# Step 1: Create a regex to extract:
#                                   - pid                   12498
#                                   - syscall name          openat
#                                   - syscall args          "AT_FCWD,...,TTY"
#                                   - syscall return value  3
# Step 2: For every syscall have a function that translates it to fstrace op
# Example: Operation ID do        ------ interpreter.ml 31, substring matching
#               newfd AT_FCWD s1.c 3
#               hpath AT_FCWD s1.c consumed
#          done
#
# trace = [pid, syscall_name, syscall_args, syscall_ret_val]
import re
import sys
from collections import namedtuple
from collections import deque
from functools import wraps
from abc import ABC,abstractmethod


################################# STRUCTURES ##################################

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
fchdir      = 'fchdir {}'             # 'fchdir' fd
symlink     = 'symlink {} {} {}'      # 'symlink' dirfd PATH PATH
link        = 'link {} {} {} {}'      # 'link' dirfd PATH dirfd PATH
rename      = 'rename {} {} {} {}'    # 'rename' dirfd PATH dirfd PATH
newfd       = 'newfd {} {} {}'        # 'newfd' dirfd PATH ret


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
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_chdir(trace):
    return [setcwd.format(trace.syscall_args[0])]


@check_paths([0])
def translate_chmod(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_chown(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


def translate_clone(trace):
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
    return [delfd.format(trace.syscall_args[0])]


def translate_dup(trace):
    return [dupfd.format(trace.syscall_args[0], trace.syscall_ret)]


def translate_dup2(trace):
    return [dupfd.format(trace.syscall_args[0], trace.syscall_args[1])]


def translate_dup3(trace):
    return [dupfd.format(trace.syscall_args[0], trace.syscall_args[1])]


@check_paths([0])
def translate_execve(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


def translate_fchdir(trace):
    return [fchdir.format(trace.syscall_args[0])]


@check_paths([1])
def translate_fchmodat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], 'consumed'
        )
    ]


@check_paths([1])
def translate_fchownat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], 'consumed'
        )
    ]


def translate_fcntl(trace):
    if 'F_DUPFD' in trace.syscall_args[1]:
        return [dupfd.format(trace.syscall_args[0], trace.syscall_ret)]
    return


def translate_fork(trace):
    return [newproc.format('none', trace.syscall_ret)]


@check_paths([0])
def translate_getxattr(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_getcwd(trace):
    return [setcwd.format(trace.syscall_args[0])]


@check_paths([0])
def translate_lchown(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_lgetxattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_lremovexattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_lsetxattr(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_lstat(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0, 1])
def translate_link(trace):
    return [
        link.format(
            'AT_FDCWD', trace.syscall_args[0],'AT_FDCWD', trace.syscall_args[1]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed'),
        hpath.format('AT_FDCWD', trace.syscall_args[1], 'produced')
    ]


@check_paths([1, 3])
def translate_linkat(trace):
    return [
        link.format(
            trace.syscall_args[0], trace.syscall_args[1],
            trace.syscall_args[2], trace.syscall_args[3]
        ),
        hpath.format(trace.syscall_args[0], trace.syscall_args[1], 'consumed'),
        hpath.format(trace.syscall_args[2], trace.syscall_args[3], 'produced')
    ]


@check_paths([0])
def translate_mkdir(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'produced')]


@check_paths([1])
def translate_mkdirat(trace):
    return [
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], 'produced'
        )
    ]


@check_paths([0])
def translate_mknod(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'produced')]


@check_paths([0])
def translate_open(trace):
    if any(x in trace.syscall_args[1] for x in ['O_CREAT', 'O_TRUNC']):
        access = 'produced'
    else:
        access = 'consumed'
    return [
        newfd.format('AT_FDCWD', trace.syscall_args[0], trace.syscall_ret),
        hpath.format('AT_FDCWD', trace.syscall_args[0], access)
    ]


@check_paths([1])
def translate_openat(trace):
    if any(x in trace.syscall_args[2] for x in ['O_CREAT', 'O_TRUNC']):
        access = 'produced'
    else:
        access = 'consumed'
    return [
        newfd.format(
            trace.syscall_args[0], trace.syscall_args[1], trace.syscall_ret
        ),
        hpath.format(
            trace.syscall_args[0], trace.syscall_args[1], access
        ),
    ]


@check_paths([0])
def translate_readlink(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([1])
def translate_readlinkat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], 'consumed'
        )
    ]


@check_paths([0])
def translate_removexattr(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0, 1])
def translate_rename(trace):
    return [
        rename.format(
            'AT_FDCWD', trace.syscall_args[0], 'AT_FDCWD', trace.syscall_args[1]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[0], 'expunged'),
        hpath.format('AT_FDCWD', trace.syscall_args[1], 'produced')
    ]



@check_paths([1, 3])
def translate_renameat(trace):
    return [
        rename.format(
            trace.syscall_args[0], trace.syscall_args[1],
            trace.syscall_args[2], trace.syscall_args[3]
        ),
        hpath.format(trace.syscall_args[0], trace.syscall_args[1], 'expunged'),
        hpath.format(trace.syscall_args[2], trace.syscall_args[3], 'produced')
    ]


@check_paths([0])
def translate_rmdir(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'expunged')]


@check_paths([0])
def translate_stat(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0])
def translate_statfs(trace):
    return [hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([0, 1])
def translate_symlink(trace):
    return [
        symlink.format(
            'AT_FDCWD', trace.syscall_args[1], trace.syscall_args[0]
        ),
        hpath.format('AT_FDCWD', trace.syscall_args[1], 'produced')

    ]


@check_paths([2])
def translate_symlinkat(trace):
    return [
        symlink.format(
            trace.syscall_args[1],
            trace.syscall_args[2],
            trace.syscall_args[0]
        ),
        hpath.format(trace.syscall_args[1], trace.syscall_args[2], 'produced')
    ]


@check_paths([0])
def translate_unlink(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'expunged')]


@check_paths([1])
def translate_unlinkat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], 'expunged'
        )
    ]


@check_paths([0])
def translate_utime(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


@check_paths([1])
def translate_utimensat(trace):
    return [
        hpathsym.format(
            trace.syscall_args[0], trace.syscall_args[1], 'consumed'
        )
    ]


@check_paths([0])
def translate_utimes(trace):
    return [hpathsym.format('AT_FDCWD', trace.syscall_args[0], 'consumed')]


def translate_write(trace):
    return [trace.syscall_args[1]]


def translate_writev(trace):
    return [trace.syscall_args[1]]

##############################  MAIN FUNCTIONS   ##############################

# trace_entries
exec_task_begin = 'execTask {} {{'      #  task_id
exec_task_end   = '}'
new_task        = 'newTask {} {}'       #  task_id task_type
depend_on       = 'dependOn {} {}'      #  task_id task_type (Gradle)
consumes        = 'consumes {} "{}"'    #  task_id string (prerequisites)
produces        = 'produces {} {}'      #  task_id string (Gradle)


def to_sysop(opexp, opid, tabs):
    ret = "{}sysop {} SYNC {{\n".format(tabs * "\t", opid)
    for op in opexp:
        ret += "{}{}\n".format( (tabs+1) * "\t", op)
    ret += (tabs * "\t") + "}"
    return ret


def write_out(out, output):
    out.write(output + '\n')


class Handler(ABC):

    def __init__(self, inp, out):
        self.inp = inp
        self.out = out
        self.unfinished = {}
        self.sys_op_id = 0

    @abstractmethod
    def _handle_write(self):
        pass

    def _handle_opexp(self, opexp):
        # Add pid
        opexp = [self.trace.pid + ", " + x for x in opexp]
        write_out(
            self.out,
            to_sysop(opexp, self.sys_op_id, 0)
        )
        self.sys_op_id += 1

    def execute(self):
        for line in self.inp:
            self.trace = parse_line(line, self.unfinished)
            if self.trace:
                opexp = globals()["translate_" + self.trace.syscall_name](self.trace)
                if self.trace.syscall_name == "write":
                    self._handle_write()
                elif opexp is not None:
                    self._handle_opexp(opexp)


class GradleHandler(Handler):
    def __init__(self, inp, out):
        super(GradleHandler, self).__init__(inp, out)

    def _handle_write(self):
        begin_reg = r'^"Begin ([A-Za-z0-9_:.-]+)'
        end_reg = r'^"End ([A-Za-z0-9_:.-]+)'
        if int(self.trace.syscall_args[0]) > 100:
            exec_task = re.search(begin_reg, self.trace.syscall_args[1])
            if exec_task:
                write_out(
                    self.out,
                    exec_task_begin.format(
                        exec_task.groups(1)[0]
                    )
                )
                return
            exec_task = re.search(end_reg, self.trace.syscall_args[1])
            if exec_task:
                write_out(
                    self.out,
                    exec_task_end
                )
                return


class MakeHandler(Handler):
    def __init__(self, inp, out):
        super(MakeHandler, self).__init__(inp, out)
        self.nesting_counter = 0  # Count how many tabs to put
        self.cwd_queue = deque()  # Save the history of cwd
        self.cwd_queue.append('')
        self.is_open_task = False
        self.end_tasks = []
        self.current_task_id = None

    def _handle_write(self):
        message = self.trace.syscall_args[1].replace('"', '').replace('\\n', '').strip()
        begin = re.search("^(.*):([0-9]+): +##BEGIN##+ (.*),(.*)", message)
        if begin:
            # target: the name of whichever target caused the rule’s recipe
            #         to be run
            makefile, line, target, prereq = begin.groups()
            prereqs = re.split(' |,', prereq)
            # In case of nested we should have the current_path
            cwd = self.cwd_queue[-1]
            cwd = cwd + "/" if cwd != '' else cwd
            task_id = "{}{}_{}_{}".format(cwd, makefile, line, target)
            if task_id == self.current_task_id:
                self.end_tasks.pop()  # Because we'll add a new one in END state
            else:
                tabs = self.nesting_counter * '\t'
                self.current_task_id = task_id
                write_out(
                    self.out,
                    tabs + new_task.format(self.current_task_id, "S 0")
                )
                for x in prereqs:
                    write_out(
                        self.out,
                        tabs + consumes.format(self.current_task_id, x)
                    )
                write_out(
                    self.out,
                    tabs + exec_task_begin.format(task_id)
                )
            self.nesting_counter += 1
            return
        if message.startswith("##END##"):
            self.end_tasks.append(exec_task_end)
            self.nesting_counter -= 1
            return
        entering = re.search("^.*:[ ]+Entering directory[ ]+'(.*)'", message)
        if entering:
            self.cwd_queue.append(entering.groups()[0])
            return
        leaving = re.search("^.*:[ ]+Leaving directory[ ]+'(.*)'", message)
        if leaving:
            self.cwd_queue.pop()
        return

    def _handle_opexp(self, opexp):
        # Add pid
        if self.is_open_task == False and len(self.end_tasks) > 0:
            write_out(
                self.out,
                (self.nesting_counter * '\t') + self.end_tasks.pop()
            )
        opexp = [self.trace.pid + ", " + x for x in opexp]
        write_out(
            self.out,
            to_sysop(opexp, self.sys_op_id, self.nesting_counter)
        )
        self.sys_op_id += 1

    def execute(self):
        super(MakeHandler, self).execute()
        if len(self.end_tasks) > 0:
            write_out(
                self.out, (self.nesting_counter * '\t') + self.end_tasks.pop()
            )


def main(inp, out, program):
    if program == "Make":
        handler = MakeHandler(inp, out)
    elif program == "Gradle":
        handler = GradleHandler(inp, out)
    handler.execute()


if __name__ == "__main__":
    program = "Gradle"
    if len(sys.argv) > 1:
        program = sys.argv[1]
    main(sys.stdin, sys.stdout, program)
