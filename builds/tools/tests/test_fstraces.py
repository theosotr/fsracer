from .. import adapter
from tempfile import NamedTemporaryFile


def test_simple_make():
    with open('tests/data/simple.traces', 'r') as f:
        traces = f.readlines()
    with open('tests/data/simple.fstraces', 'r') as f:
        expected = f.readlines()
    with NamedTemporaryFile('w') as f:
        adapter.main(traces, f, adapter.parse_make_write)
        f.flush()
        with open(f.name, 'r') as inp:
            fstraces = inp.readlines()
        for exp, out in zip(expected, fstraces):
            assert exp.replace(4 * ' ', '\t') == out
