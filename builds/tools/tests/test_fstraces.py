from .. import adapter
from tempfile import NamedTemporaryFile


def helper_fun(in_traces, fstraces):
    with open(in_traces, 'r') as f:
        traces = f.readlines()
    with open(fstraces, 'r') as f:
        expected = f.readlines()
    with NamedTemporaryFile('w') as f:
        adapter.main(traces, f, "Make")
        f.flush()
        with open(f.name, 'r') as inp:
            fstraces = inp.readlines()
        for exp, out in zip(expected, fstraces):
            assert exp.replace(4 * ' ', '\t') == out



def test_simple_make():
    helper_fun('tests/data/simple.traces', 'tests/data/simple.fstraces')


def test_multiline_make():
    helper_fun('tests/data/multiline.traces', 'tests/data/multiline.fstraces')


def test_nested_make():
    helper_fun('tests/data/nested.traces', 'tests/data/nested.fstraces')
