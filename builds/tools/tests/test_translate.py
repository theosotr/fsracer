from .. import adapter


TRACES = {
    'access': (
        '6413 access("/etc/ld.so.nohwcap", F_OK) = -1 ENOENT (No such file or directory)', ['hpath AT_FDCWD "/etc/ld.so.nohwcap" consumed']
    )
}

temp = {}

def test_translate_functions():
    for syscall, values in TRACES.items():
        translate_method = getattr(adapter, 'translate_' + syscall)
        trace_line = values[0]
        opexprs = values[1]
        trace = adapter.parse_line(trace_line, temp)
    assert(translate_method(trace) == opexprs)
