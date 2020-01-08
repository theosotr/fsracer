#! /usr/bin/env python3

import re
import sys

regex = re.compile('\[Task: (.*)\]')

def count_bugs(path):
    bugs = {
        'MIN': 0,
        'MOUT': 0,
        'MOR': 0
    }
    task = None
    with open(path) as f:
        for line in f:
            line = line.strip()
            search = re.search(regex, line)
            if search is not None:
                task = search.group(1)
                continue
            if line.startswith('Fault Type'):
                _, fault_type = line.split(': ')
                assert task is not None
                bugs[fault_type] += 1
                task = None
    print (bugs['MIN'])
    print (bugs['MOUT'])
    print (bugs['MOR'])

count_bugs(sys.argv[1])
