import re

# REGEX cannot handle recursion
def safe_spliti_regex(string):
    # split with ,
    # lookups
    # ,\s*(?![^\[\]]*\])(?![^()]*\))(?![^\{\}]*\})
    # ,   : match ','
    # \s* : any number of whitespace characters
    # ?!  : negative lookahead, the part within `(?!...)` must not match
    # (?![^\[\]]*\]) : for '[]'
    #    [^\[\]]*   : 0 or more non '[]' characters
    #            \] : 1 ']'
    # (?![^()]*\))   : for ()
    #    [^()]*     : 0 or more non '()' characters
    #          \)   : 1 ')'
    # (?![^\{\}]*\}) : for {}
    #    [^\{\}]*   : 0 or more non '{}' characters
    #            \} : 1 '}'
    return re.split(
        r',\s*(?![^\[\]]*\])(?![^()]*\))(?![^\{\}]*\})',
        string
    )

matches = {
    '[': ']', '(': ')', '{': '}', '"': '"'
}

def safe_split(string):
    index = 0
    inside = None
    res = []
    for counter, c in enumerate(string):
        if c in ('[', '(', '{', '"') and not inside:
            inside = c
            continue
        if inside and c == matches[inside]:
            inside = None
            continue
        if c == ',' and not inside:
            res.append(string[index:counter].strip())
            index = counter + 1
    res.append(string[index:len(string)].strip())  # Add last element
    return res


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
# REGEX fails
assert safe_split(tests[9]) == ['"/usr/bin/make"', '["make", ["foo", "bar"]]']
# REGEX fails
assert safe_split(tests[10]) == ['"bar"', '"foo,bar"']
