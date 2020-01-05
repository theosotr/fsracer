#! /usr/bin/env python3


import fileinput

times = []
for line in fileinput.input():
    _, atime, btime = line.split(',')
    times.append(float(atime) + float(btime))


print (sum(times) / len(times))
