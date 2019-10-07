from ..adapter import parse_line, translate_access

TRACES = [
    '6413 access("/etc/ld.so.nohwcap", F_OK) = -1 ENOENT (No such file or directory)'
]

temp = {}

def test_translate_access():
    trace = parse_line(TRACES[0], temp)
    assert(translate_access(trace)[0] ==
           'hpath AT_FDCWD "/etc/ld.so.nohwcap" consumed'
          )
