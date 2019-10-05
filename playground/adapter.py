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
import re
import sys

##############################     TEST DATA     ##############################

TRACES = [
    '31565 access("/etc/ld.so.nohwcap", F_OK) = -1 ENOENT (No such file or directory)'
]

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
    syscall_unfinished_re = r'^{}\({}[ ]+<unfinished ...>'.format(
        "([a-z0-9_]+)", "(.*)"
    )
    syscall_unfinished = re.search(syscall_unfinished_re, trace)
    if syscall_unfinished:
        return syscall_unfinished.groups()
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
        (syscall_name, syscall_args, syscall_ret_val) or None
    Example:
     1)
        input: "openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "<... rt_sigprocmask resumed> NULL, 8) = 0"
        returns: ('rt_sigprocmask', 'NULL, 8', '0')
    """
    syscall_res_re =\
        r'^<...[ ]+{}[ ]+resumed>[ ]*{}\)[ ]*=[ ]*{}[ ]*.*?'.format(
            "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
        )
    syscall_resumed = re.search(syscall_res_re, trace)
    if syscall_resumed:
        return syscall_resumed.groups()
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
        syscall_re_groups = re.search(syscall_re, trace).groups()
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None
    return syscall_re_groups


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
        return (pid, syscall_name, syscall_args, syscall_ret_val)
        del unfinished[(pid, syscall_name)]
    else:
        temp_trace = parse_trace(trace)
        if temp_trace:
            return (pid,) + temp_trace

##############################  MAIN FUNCTIONS   ##############################

def main(inp):
    traces = []
    unfinished = {}
    for line in inp:
        trace = parse_line(line, unfinished)
        if trace:
            print(trace)
            traces.append(trace)
    print(len(traces))


if __name__ == "__main__":
    test = TRACES if len(sys.argv) > 1 and sys.argv[1] == "test" else sys.stdin
    main(test)
