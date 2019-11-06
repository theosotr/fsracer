import time
import queue 
import sys
import argparse
from multiprocessing import Lock, Process, Queue, current_process


def fsmake(source_name):
    # Return straces path
    # Run docker image and detect if .straces
    pass

def adapter(straces_path):
    # Return fstraces path
    # Run adapter.py and detect if .fstraces
    pass

def fsracer(fstraces_path):
    # Return package name 
    # Run fsracer and add True or False
    pass

# Task should be a function
def do_work(in_queue, out_queue, task):
    while True:
        try:
            # try to get task from the queue. get_nowait() function will 
            # raise queue.Empty exception if the queue is empty. 
            # queue(False) function would do the same task also.
            element = in_queue.get_nowait()
        except queue.Empty:
            break
        else:
            # if no exception has been raised, execute the task
            # if task return not None add it to out_queue
            res = task(element)
            if res is not None:
                out_queue.put(res)
    return True


def main(inp, make, adapter, fsracer):
    number_of_processes_1 = make
    number_of_processes_2 = adapter
    number_of_processes_3 = fsracer
    # Elements: source_name, X (e.g. source_name,straces_path)
    q1 = Queue() # sources
    q2 = Queue() # straces
    q3 = Queue() # fstraces
    q4 = Queue() # result
    processes1 = []
    processes2 = []
    processes3 = []

    for i in inp:
        q1.put(i)

    # creating processes
    for w in range(number_of_processes_1):
        p = Process(target=do_work, args=(q1, q2, fsmake))
        processes1.append(p)
        p.start()

    for w in range(number_of_processes_2):
        p = Process(target=do_work, args=(q2, q3, adapter))
        processes2.append(p)
        p.start()

    for w in range(number_of_processes_3):
        p = Process(target=do_work, args=(q3, q4, fsracer))
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
    parser.add_argument('make_processes', type=int)
    parser.add_argument('adapter_processes', type=int)
    parser.add_argument('fsracer_processes', type=int)
    args = parser.parse_args()
    inp = sys.stdin
    if args.input:
        with open(args.input, 'r') as f:
            inp = f.readlines()
    main(inp, make_processes, adapter_processes, fsracer_processes)
