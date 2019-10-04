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
from pprint import pprint


def match_unfinished(trace):
    """Match unfinished trace line.

    We use the following regex to match unfinished traces

    ^([0-9]+)[ ]+([a-z0-9_]+)\((.*)[ ]+<unfinished ...>

    If it matches return pid, system call name, and system call arguments.
    Otherwise, return None.

    Args:
        trace: string

    Returns:
        [pid, syscall_name, syscall_args] or None

    Example:
     1)
        input: "12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "12496 rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>"
        returns: ['12496', 'rt_sigprocmask', SIG_SETMASK, [],]
    """
    syscall_unfinished_re = r'^{}[ ]+{}\({}[ ]+<unfinished ...>'.format(
        "([0-9]+)", "([a-z0-9_]+)", "(.*)"
    )
    syscall_unfinished = re.search(syscall_unfinished_re, trace)
    if syscall_unfinished:
        return syscall_unfinished.groups()
    return None


def match_resumed(trace):
    """Match resumed trace line.

    We use the following regex to match resumed traces

    ^([0-9]+)[ ]+<...[ ]+([a-z0-9_]+)[ ]+resumed>[ ]*(.*)\)[ ]*=[ ]*(-?[0-9\?]+)[ ]*.*?

    If it matches return system call name, rest of system call arguments, and
    system call return value. Otherwise, return None.

    Args:
        trace: string

    Returns:
        [pid, syscall_name, syscall_args, syscall_ret_val] or None

    Example:
     1)
        input: "12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: None
    2)
        input: "12496 <... rt_sigprocmask resumed> NULL, 8) = 0"
        returns: ['12496', 'rt_sigprocmask', 'NULL, 8', '0']
    """
    syscall_res_re =\
        r'^{}[ ]+<...[ ]+{}[ ]+resumed>[ ]*{}\)[ ]*=[ ]*{}[ ]*.*?'.format(
            "([0-9]+)", "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
        )
    syscall_resumed = re.search(syscall_res_re, trace)
    if syscall_resumed:
        return syscall_resumed.groups()
    return None


def parse_trace(trace):
    """Parse a trace on strace's format.

    We use the following regex to match pid, system call name, system call
    arguments, and system call return value. The four groups of the regex
    match pid, syscall name, syscall args, and syscall return value.

    ^([0-9]+)[ ]+([a-z0-9_]+)\((.*)\)[ ]*=[ ]*(-?[0-9\?]+)[ ]*.*?

    Args:
        trace: str

    Returns:
        [pid, syscall_name, syscall_args, syscall_ret_val]

    Example:
     1)
        input: "12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3"
        returns: ['12498', 'openat', 'AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY', '3']
     2)
        input: "12499 access("/etc/ld.so.preload", R_OK) = -1 ENOENT (No such file or directory)"
        returns: ['12498', 'access', 'openat', '"/etc/ld.so.preload", R_OK', '-1']
    """

    syscall_re = r'^{}[ ]+{}\({}\)[ ]*=[ ]*{}[ ]*.*?'.format(
        "([0-9]+)", "([a-z0-9_]+)", "(.*)", "(-?[0-9\?]+)"
    )
    try:
        syscall_re_groups = re.search(syscall_re, trace).groups()
    except AttributeError:
        print("AttributeError occured: " + trace, file=sys.stderr)
        return None
    return syscall_re_groups


def main():
    traces = []
    unfinished = {}
    for trace_line in sys.stdin:
        trace_line = trace_line.rstrip()
        if any(x in trace_line for x in ['---', '+++']):  # Skip Signals
            pass
        elif '<unfinished ...>' in trace_line:
            pid, syscall_name, syscall_args = match_unfinished(trace_line)
            # Warning: What happens if there are two unfinished system call
            # from the same pid with the same system call names?
            unfinished[(pid, syscall_name)] = syscall_args
        elif ' resumed>' in trace_line:
            pid, syscall_name, rest_syscall_args, syscall_ret_val =\
                match_resumed(trace_line)
            syscall_args = unfinished[(pid, syscall_name)] + rest_syscall_args
            traces.append((pid, syscall_name, syscall_args, syscall_ret_val))
            del unfinished[(pid, syscall_name)]
        else:
            trace = parse_trace(trace_line)
            if trace:
                traces.append(trace)
    pprint(traces)
    print(len(traces))


if __name__ == "__main__":
    main()
