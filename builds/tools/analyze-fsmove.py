#! /usr/bin/env python3

import re
import sys

regex = re.compile('- (.*): Consumed by.*')

def get_input_files(path):
    ignore = True
    files = set()
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('Fault Type:'):
                if 'MIN' in line:
                    ignore = False
                else:
                    ignore = True
            if line.startswith('-') and not ignore:
                search = re.search(regex, line)
                files.add(search.group(1))
    for f in sorted(files):
        print (f)


get_input_files(sys.argv[1])
