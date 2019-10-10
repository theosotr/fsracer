from .. import adapter
import json


with open('tests/data/traces.json', 'r') as f:
    TRACES = json.load(f)

temp = {}

def test_translate_functions():
    for syscall, tests in TRACES.items():
        if syscall in ('fork', 'lgetxattr', 'linkat', 'mknod', 'readlinkat',
                       'lremovexattr', 'lsetxattr', 'removexattr', 'renameat',
                       'symlinkat', 'write', 'writev'):
            continue
        translate_method = getattr(adapter, 'translate_' + syscall)
        for test in tests:
            trace_line = test[0]
            opexprs = test[1]
            print("\nDEBUG: " + syscall + ": " + ';' + trace_line + ';')
            trace = adapter.parse_line(trace_line, temp)
            assert(translate_method(trace) == opexprs)
