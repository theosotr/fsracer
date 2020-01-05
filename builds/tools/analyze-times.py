import statistics


m_times = {}
with open('mkcheck.times') as f:
    for line in f:
        project, time = line.split(' ')
        m_times[project] = float(time)

f_times = {}
with open('fsmove.times') as f:
    for line in f:
        project, time = line.split(' ')
        f_times[project] = float(time)


times = []
for p, m_time in m_times.items():
    f_time = f_times[p]
    print (p, m_time, f_time)
    times.append(m_time / f_time)

print (sum(times) / len(times), statistics.median(times))
