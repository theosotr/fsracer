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
    found = False
    with open(path) as f:
        for line in f:
            line = line.strip()
            search = re.search(regex, line)
            if search is not None:
                task = search.group(1)
                continue
            if line.startswith('Fault Type'):
                found = True
                _, fault_type = line.split(': ')
                bugs[fault_type] += 1
                task = None
    if found:
        print (bugs['MIN'], bugs['MOUT'], bugs['MOR'])

count_bugs(sys.argv[1])
