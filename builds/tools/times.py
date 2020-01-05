#! /usr/bin/env python3


import fileinput

times_p = []
for line in fileinput.input():
    _, times = line.split(',', 1)
    times = [float(x) for x in times.split(',')]
    times_p.append(sum(times))


print (sum(times_p) / len(times_p))
