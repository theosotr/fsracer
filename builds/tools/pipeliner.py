#!/usr/bin/env python3
import sys
import os
import argparse
import subprocess as sp
from multiprocessing import Lock, Process, Queue, current_process


DEBIAN_DOCKER_IMAGE = ""
ADAPTER = ""
FSRACER_DOCKER_IMAGE = ""
DEBIAN_TIMEOUT = 0
FSRACER_TIMEOUT = 0

printlock = Lock()

def file_exists(path):
    """Check if file exists and is not empty"""
    return os.path.isfile(path) and os.path.getsize(path) > 0


def mprint(ptype, pname, message, sname):
    with printlock:
        print("{}-{}: {}: {}".format(ptype, pname, message, sname))
        sys.stdout.flush()


def fsmake(source_name, volume):
    # Run docker image and detect if .straces
    mprint("fsmake", current_process().name, "consumed", source_name)
    docker_options = [
        'docker',
        'run',
        '--rm',
        '-v',
        volume + ':/var/log/sbuild/straces',
        '--cap-add',
        'SYS_ADMIN',
        'sbuild_fsmake',
        source_name
    ]
    sp.run(
        docker_options, stdout=sp.PIPE, stderr=sp.PIPE, timeout=DEBIAN_TIMEOUT
    )
    #  cmd.communicate()
    path = '{}/{}/{}'.format(volume, source_name, source_name)
    if file_exists(path + '.straces') and file_exists(path + '.path'):
        mprint("fsmake", current_process().name, "success", source_name)
        return source_name
    mprint("fsmake", current_process().name, "failed", source_name)
    return None


def adapter(source_name, volume):
    # Run adapter.py and detect if .fstraces
    mprint("adapter", current_process().name, "consumed", source_name)
    path = '{}/{}/{}'.format(volume, source_name, source_name)
    with open(path + '.path', 'r') as f:
        pwd = f.read().strip()
    adapter_options = [
        'adapter.py',
        '-c',
        'make',
        pwd
    ]
    with open(path + '.straces', 'rb') as inf:
        with open(path + '.fstraces', 'wb') as outf:
            with open(path + '.aderr', 'wb') as errf:
                cmd = sp.Popen(
                    adapter_options, stdin=inf, stdout=outf, stderr=errf
                )
                cmd.communicate()
    if file_exists(path + '.straces'):
        sys.stdout.flush()
        mprint("adapter", current_process().name, "success", source_name)
        return source_name
    mprint("adapter", current_process().name, "failed", source_name)
    return source_name


def fsracer(source_name, volume):
    # Run fsracer and add True or False
    mprint("fsracer", current_process().name, "consumed", source_name)
    path = '{}/{}/{}'.format(volume, source_name, source_name)
    docker_options = [
        'docker',
        'run',
        '--rm',
        '-v',
        volume + '/' + source_name + ':/home/fsracer/' + source_name,
        'fsracer',
        'run_fsracer.sh',
        source_name
    ]
    sp.run(
        docker_options, stdout=sp.PIPE, stderr=sp.PIPE, timeout=FSRACER_TIMEOUT
    )
    #  cmd.communicate()
    path = '{}/{}/{}'.format(volume, source_name, source_name)
    if file_exists(path + '.faults'):
        mprint("fsracer", current_process().name, "success", source_name)
        return source_name
    mprint("fsracer", current_process().name, "failed", source_name)
    return None

# Task should be a function
def do_work(in_queue, out_queue, volume, task, prev_pool):
    while True:
        val = in_queue.get()
        # TODO kill processes when done
        # If we read -1 it means there aren't other packages in the queue
        #  if val == "-1":
            # If prev_pool is empty it means there won't be new packages in
            # in_queue
            #  if len(prev_pool) == 0:
                # To stop processes from the next poll
                #  out_queue.put("-1")
            # To stop next process
            #  in_queue.put("-1")
            #  break
        res = task(val, volume)
        if res is not None:
            out_queue.put(res)
    return True


def main(inp, volume, make_p, adapter_p, fsracer_p):
    number_of_processes_1 = make_p
    number_of_processes_2 = adapter_p
    number_of_processes_3 = fsracer_p
    # Elements: source_name
    q1 = Queue() # sources
    q2 = Queue() # straces
    q3 = Queue() # fstraces
    q4 = Queue() # result
    processes1 = []
    processes2 = []
    processes3 = []

    for i in inp:
        q1.put(i.strip())
    q1.put("-1")

    # creating processes
    for _ in range(number_of_processes_1):
        p = Process(target=do_work, args=(q1, q2, volume, fsmake, []))
        processes1.append(p)
        p.start()

    for _ in range(number_of_processes_2):
        p = Process(target=do_work, args=(q2, q3, volume, adapter, processes1))
        processes2.append(p)
        p.start()

    for _ in range(number_of_processes_3):
        p = Process(target=do_work, args=(q3, q4, volume, fsracer, processes2))
        processes3.append(p)
        p.start()

    # completing process
    for p in processes1:
        p.join()

    for p in processes2:
        p.join()

    for p in processes3:
        p.join()

    # print pakcages that we have successfully analyzed
    while not q4.empty():
        print(q4.get())

    return True


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input')
    parser.add_argument('-d', '--debian-docker-image', default='sbuild_fsmake')
    parser.add_argument('-a', '--adapter', default='adapter.py')
    parser.add_argument('-f', '--fsracer-docker-image', default='fsracer')
    parser.add_argument('-t', '--debian-timeout', type=int, default=1800)
    parser.add_argument('-T', '--fsracer-timeout', type=int, default=1800)
    parser.add_argument('volume_dir')
    parser.add_argument('make_processes', type=int)
    parser.add_argument('adapter_processes', type=int)
    parser.add_argument('fsracer_processes', type=int)
    args = parser.parse_args()
    DEBIAN_DOCKER_IMAGE = args.debian_docker_image
    ADAPTER = args.adapter
    FSRACER_DOCKER_IMAGE = args.fsracer_docker_image
    DEBIAN_TIMEOUT = args.debian_timeout
    FSRACER_TIMEOUT = args.fsracer_timeout
    inp = sys.stdin
    if args.input:
        with open(args.input, 'r') as f:
            inp = f.readlines()
    main(
        inp,
        args.volume_dir,
        args.make_processes,
        args.adapter_processes,
        args.fsracer_processes
    )
