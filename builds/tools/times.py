#! /usr/bin/env python3


import fileinput

times = []
for line in fileinput.input():
    _, times = line.split(',')
    times = [float(x) for x in times.split(',')]
    times.append(sum(times))


print (sum(times) / len(times))
