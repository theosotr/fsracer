from .. import adapter
from tempfile import NamedTemporaryFile


PATH = '/home/user/fsracer/builds/make-plugin/playground/'

def helper_fun(in_traces, fstraces, makedb, example):
    work_dir = PATH + example
    with open(in_traces, 'r') as f:
        traces = f.readlines()
    with open(fstraces, 'r') as f:
        expected = f.readlines()
    with NamedTemporaryFile('w') as f:
        adapter.main(traces, f, "make", work_dir, makedb)
        f.flush()
        with open(f.name, 'r') as inp:
            fstraces = inp.readlines()
        for exp, out in zip(expected, fstraces):
            assert exp.replace(4 * ' ', '\t') == out



def test_simple_make():
    helper_fun(
        'tests/data/simple.traces', 
        'tests/data/simple.fstraces', 
        'tests/data/simple.makedb', 
        'simple_example'
    )


def test_multiline_make():
    helper_fun(
        'tests/data/multiline.traces',
        'tests/data/multiline.fstraces',
        'tests/data/multiline.makedb',
        'multiple_lines'
    )


def test_nested_make():
    helper_fun(
        'tests/data/nested.traces',
        'tests/data/nested.fstraces',
        'tests/data/nested.makedb',
        'nested_example'
    )


#  def test_complex_nested():
    #  helper_fun(
        #  'tests/data/complex_nested.traces',
        #  'tests/data/complex_nested.fstraces',
        #  'complex_nested'
    #  )

def test_anna():
    helper_fun(
        'tests/data/anna.traces',
        'tests/data/anna.fstraces',
        'tests/data/anna.makedb',
        'anna'
    )

def test_include():
    helper_fun(
        'tests/data/include.traces',
        'tests/data/include.fstraces',
        'tests/data/include.makedb',
        'include'
    )
