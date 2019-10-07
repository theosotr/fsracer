import re
from ..adapter import safe_split, split_line, parse_unfinished, parse_resumed,\
        parse_trace, parse_line, Trace


STRINGS = [
    '"foo", "bar"',
    '"/usr/bin/make", ["make", "SHELL=$(info \\#BEGIN)$(warning $*,$@,$^)./post-shell"], 0x7ffdb4e8c430 /* 24 vars */',
    '"/etc/ld.so.nohwcap", F_OK',
    '"/home/user/fsracer/playground", 4096',
    '"/proc/self/fd/1", "/dev/pts/0", 4095',
    '"/dev/pts/0", {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}',
    'AT_FDCWD, ".", O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY',
    '3',
    '"/usr/bin/make", ["make"], ["foo"]',
    '"/usr/bin/make", ["make", ["foo", "bar"]]',
    '"bar", "foo,bar"'
]

TRACE_LINES = [
    '12498 openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3',
    '12499 rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>',
    '12499 <... rt_sigprocmask resumed> NULL, 8) = 0'
]

TRACES = [
    'openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3',
    'rt_sigprocmask(SIG_SETMASK, [],  <unfinished ...>',
    '<... rt_sigprocmask resumed> NULL, 8) = 0',
    'access("/etc/ld.so.preload", R_OK) = -1 ENOENT (No such file or directory)'
]

def test_safe_split():
    assert(safe_split(STRINGS[0]) == ['"foo"', '"bar"'])
    assert(safe_split(STRINGS[1]) == [
        '"/usr/bin/make"',
        '["make", "SHELL=$(info \\#BEGIN)$(warning $*,$@,$^)./post-shell"]',
        '0x7ffdb4e8c430 /* 24 vars */'
    ])
    assert(safe_split(STRINGS[2]) == ['"/etc/ld.so.nohwcap"', 'F_OK'])
    assert(safe_split(STRINGS[3]) ==
           ['"/home/user/fsracer/playground"', '4096']
          )
    assert(safe_split(STRINGS[4]) ==
           ['"/proc/self/fd/1"', '"/dev/pts/0"', '4095']
          )
    assert(safe_split(STRINGS[5]) == [
        '"/dev/pts/0"',
        '{st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}'
    ])
    assert(safe_split(STRINGS[6]) == [
        'AT_FDCWD',
        '"."',
        'O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY'
    ])
    assert(safe_split(STRINGS[7]) == ['3'])
    assert(safe_split(STRINGS[8]) ==
           ['"/usr/bin/make"', '["make"]', '["foo"]']
          )
    assert(safe_split(STRINGS[9]) ==
           ['"/usr/bin/make"', '["make", ["foo", "bar"]]']
          )
    assert(safe_split(STRINGS[10]) == ['"bar"', '"foo,bar"'])

def test_split_line():
    assert(split_line(TRACE_LINES[0]) ==
           ('12498', 'openat(AT_FDCWD, "s1.c", O_RDONLY|O_NOCTTY) = 3')
          )

def test_parse_unfinished():
    assert(parse_unfinished(TRACES[0]) is None)
    assert(parse_unfinished(TRACES[1]) ==
           ('rt_sigprocmask', 'SIG_SETMASK, [], ')
          )

def test_parse_resumed():
    assert(parse_resumed(TRACES[0]) is None)
    assert(parse_resumed(TRACES[2]) == ['rt_sigprocmask', ['NULL', '8'], '0'])

def test_parse_test():
    assert(parse_trace(TRACES[0]) ==
           ['openat', ['AT_FDCWD', '"s1.c"', 'O_RDONLY|O_NOCTTY'], '3']
          )
    assert(parse_trace(TRACES[3]) ==
           ['access', ['"/etc/ld.so.preload"', 'R_OK'], '-1']
          )

def test_parse_line():
    unfinished = {}
    trace1 = parse_line(TRACE_LINES[0], unfinished)
    assert(unfinished == {})
    assert(trace1 ==
           Trace('12498', 'openat',
                 ['AT_FDCWD', '"s1.c"', 'O_RDONLY|O_NOCTTY'], '3'
                )
          )
    trace2 = parse_line(TRACE_LINES[1], unfinished)
    assert(unfinished == {('12499', 'rt_sigprocmask'): 'SIG_SETMASK, [], '})
    assert(trace2 is None)
    trace3 = parse_line(TRACE_LINES[2], unfinished)
    assert(unfinished == {})
    # TODO: Inspect first element of arguments
    assert(trace3 ==
           Trace('12499', 'rt_sigprocmask',
                 ['SIG_SETMASK, [], ', 'NULL', '8'], '0'
                )
          )
