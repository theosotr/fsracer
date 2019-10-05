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
import collections

##############################     TEST DATA     ##############################

TRACES = [
    '31565 access("/etc/ld.so.nohwcap", F_OK) = -1 ENOENT (No such file or directory)',
    '14333 clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7fc298fcfa10) = 14334',
    '14352 close(3) = 0'
]

################################# STRUCTURES ##################################

MATCHES = {
    '[': ']', '(': ')', '{': '}', '"': '"'
}

# A Trace is composed of process id, system call name, system call arguments,
# and system call return value.
# pid,name,ret are strings, args is a list of strings
Trace = collections.namedtuple(
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


def match_unfinished(trace):
    """Match unfinished trace.
    We use the following regex to match unfinished traces
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
        returns: ('rt_sigprocmask', SIG_SETMASK, [],)
    """
    syscall_re = r'^{}\({}[ ]+<unfinished ...>'.format(
        "([a-z0-9_]+)", "(.*)"
    )
    try:
        return re.search(syscall_re, trace).groups()
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None


def match_resumed(trace):
    """Match resumed trace.
    We use the following regex to match resumed traces
    ^<...[ ]+([a-z0-9_]+)[ ]+resumed>[ ]*(.*)\)[ ]*=[ ]*(-?[0-9\?]+)[ ]*.*?
    If it matches return system call name, rest of system call arguments, and
    system call return value. Otherwise, return None.
    Args:
        trace: string
    Returns:
        [syscall_name, syscall_args, syscall_ret_val] or None
    Example:
     1)
        input: "openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "<... rt_sigprocmask resumed> NULL, 8) = 0"
        returns: ('rt_sigprocmask', 'NULL, 8', '0')
    """
    syscall_re =\
        r'^<...[ ]+{}[ ]+resumed>[ ]*{}\)[ ]*=[ ]*{}[ ]*.*?'.format(
            "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
        )
    try:
        res = list(re.search(syscall_re, trace).groups())
        res[1] = safe_split(res[1])
        return res
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
        returns: ['openat', 'AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY', '3']
     2)
        input: "access("/etc/ld.so.preload", R_OK) = -1 ENOENT (No such file or directory)"
        returns: ['access', 'openat', '"/etc/ld.so.preload", R_OK', '-1']
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
        input: '12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3', traces, input
        result:
            return [('12498', 'openat', 'AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY', '3')]
            unfinished = {}
        input: '12499 rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>', traces, input
        result:
            return None
            unfinished = {(12499, rt_sigprocmask): 'SIG_SETMASK, [], '}
        input: '12499 <... rt_sigprocmask resumed> NULL, 8) = 0', traces, input
        result:
            return ('12499', 'rt_sigprocmask', 'SIG_SETMASK, [], NULL, 8', '0')
            unfinished = {}
    """
    line = line.rstrip()
    pid, trace = split_line(line)
    if any(trace.startswith(x) for x in ['---', '+++']):  # Skip Signals
        pass
    elif trace.endswith('>'):
        syscall_name, syscall_args = match_unfinished(trace)
        # Warning: What happens if there are two unfinished system call
        # from the same pid with the same system call names?
        unfinished[(pid, syscall_name)] = syscall_args
    elif trace.startswith('<'):
        syscall_name, rest_syscall_args, syscall_ret_val =\
                match_resumed(trace)
        syscall_args = unfinished[(pid, syscall_name)] + rest_syscall_args
        return Trace(pid, syscall_name, syscall_args, syscall_ret_val)
        del unfinished[(pid, syscall_name)]
    else:
        temp_trace = parse_trace(trace)
        if temp_trace:
            return Trace(pid, temp_trace[0], temp_trace[1], temp_trace[2])

############################# TRANSLATE FUNCTIONS #############################

# def translate_<system_call_name>(trace):
#   return [operation_expression, ...]

hpath   = 'hpath {} {} {}'      # 'hpath' dirfd PATH access
setcwd  = 'setcwd {}'           # 'setcwd' PATH
newproc = 'newproc {} {}'       # 'newproc' ['fd'/'fs'/'fdfs'/'none'] ret
delfd   = 'delfd {}'            # 'delfd' fd
dupfd   = 'dupfd {} {}'         # 'dupfd' fd fd


def translate_access(trace):
    return hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')


def translate_chdir(trace):
    return setcwd.format(trace.syscall_args[0])


def translate_chmod(trace):
    return hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')


def translate_chown(trace):
    return hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')


def translate_clone(trace):
    if all(x in trace.syscall_args[1] for x in ['CLONE_FS', 'CLONE_FILES']):
        return newproc.format('fdfs', trace.syscall_ret)
    elif 'CLONE_FS' in trace.syscall_args[1]:
        return newproc.format('fs', trace.syscall_ret)
    elif 'CLONE_FILES' in trace.syscall_args[1]:
        return newproc.format('fd', trace.syscall_ret)
    else:
        return newproc.format('none', trace.syscall_ret)


def translate_close(trace):
    return delfd.format(trace.syscall_args[0])


def translate_dup(trace):
    return dupfd.format(trace.syscall_args[0], trace.syscall_ret)


def translate_dup2(trace):
    return dupfd.format(trace.syscall_args[0], trace.syscall_args[1])


def translate_dup3(trace):
    return dupfd.format(trace.syscall_args[0], trace.syscall_args[1])


def translate_execve(trace):
    return hpath.format('AT_FDCWD', trace.syscall_args[0], 'consumed')


def translate_fchdir(trace):
    return


def translate_fchmodat(trace):
    return


def translate_fchownat(trace):
    return


def translate_fcntl(trace):
    return


def translate_fork(trace):
    return


def translate_getxattr(trace):
    return


def translate_getcwd(trace):
    return


def translate_lchown(trace):
    return


def translate_lgetxattr(trace):
    return


def translate_lremovexattr(trace):
    return


def translate_lsetxattr(trace):
    return


def translate_lstat(trace):
    return


def translate_link(trace):
    return


def translate_linkat(trace):
    return


def translate_mkdir(trace):
    return


def translate_mkdirat(trace):
    return


def translate_mknod(trace):
    return


def translate_open(trace):
    return


def translate_openat(trace):
    return


def translate_readlink(trace):
    return


def translate_readlinkat(trace):
    return


def translate_removexattr(trace):
    return


def translate_rename(trace):
    return


def translate_renameat(trace):
    return


def translate_rmdir(trace):
    return


def translate_stat(trace):
    return


def translate_statfs(trace):
    return


def translate_symlink(trace):
    return


def translate_symlinkat(trace):
    return


def translate_unlink(trace):
    return


def translate_unlinkat(trace):
    return


def translate_utime(trace):
    return


def translate_utimensat(trace):
    return


def translate_utimes(trace):
    return


def translate_write(trace):
    return

##############################  MAIN FUNCTIONS   ##############################

def main(inp):
    traces = []
    operation_expressions = []
    unfinished = {}
    for line in inp:
        trace = parse_line(line, unfinished)
        if trace:
            print(80*"#")
            print(trace)
            traces.append(trace)
            opexp = globals()["translate_" + trace.syscall_name](trace)
            print(opexp)
            operation_expressions.append(opexp)
            print(80*"#")
    print(len(traces))


if __name__ == "__main__":
    test = TRACES if len(sys.argv) > 1 and sys.argv[1] == "test" else sys.stdin
    main(test)
