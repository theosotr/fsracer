#! /usr/bin/env python3

import sys


def get_input_files(path):
    files = set()
    in_file = None
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('['):
                in_file = line.split(']')[1].strip()

            if in_file is None:
                continue
            if line.startswith('-'):
                if not in_file.endswith('libtool:') and not in_file.endswith('/config.h:'):
                    files.add(in_file[:-1])
    for f in sorted(files):
        print(f)


get_input_files(sys.argv[1])