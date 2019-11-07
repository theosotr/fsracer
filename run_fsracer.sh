#! /bin/sh

package=$1
path=/home/fsracer/$package
mkdir -p $path

/home/fsracer/fsracer/build/tools/fsracer/fsracer -i $path/$package.fstraces \
    --fault-detector fs > $path/$package.faults 2> $path/$package.fserr
