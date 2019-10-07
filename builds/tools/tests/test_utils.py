import re
from ..adapter import safe_split


tests = [
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

def test_safe_split():
    assert safe_split(tests[0]) == ['"foo"', '"bar"']
    assert safe_split(tests[1]) == [
        '"/usr/bin/make"',
        '["make", "SHELL=$(info \\#BEGIN)$(warning $*,$@,$^)./post-shell"]',
        '0x7ffdb4e8c430 /* 24 vars */'
    ]
    assert safe_split(tests[2]) == ['"/etc/ld.so.nohwcap"', 'F_OK']
    assert safe_split(tests[3]) == ['"/home/user/fsracer/playground"', '4096']
    assert safe_split(tests[4]) == ['"/proc/self/fd/1"', '"/dev/pts/0"', '4095']
    assert safe_split(tests[5]) == [
        '"/dev/pts/0"',
        '{st_mode=S_IFCHR|0620, st_rdev=makedev(136, 0), ...}'
    ]
    assert safe_split(tests[6]) == [
        'AT_FDCWD',
        '"."',
        'O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY'
    ]
    assert safe_split(tests[7]) == ['3']
    assert safe_split(tests[8]) == ['"/usr/bin/make"', '["make"]', '["foo"]']
    assert safe_split(tests[9]) == ['"/usr/bin/make"', '["make", ["foo", "bar"]]']
    assert safe_split(tests[10]) == ['"bar"', '"foo,bar"']
