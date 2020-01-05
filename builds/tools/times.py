#! /usr/bin/env python3


import fileinput
import statistics
import sys

times_p = []
for line in fileinput.input():
    p, times = line.split(',', 1)
    times = [float(x) for x in times.split(',')]
    p_time = sum(times)
    times_p.append(p_time)
    print (p, p_time)


print (sum(times_p) / len(times_p), statistics.median(times_p), file=sys.stderr)
