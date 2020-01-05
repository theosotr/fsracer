#! /bin/bash

for file in $1/*; do
    project=$(basename $file)
    if [ ! -f $file/$project.time ]; then
        continue
    fi
    atime=$(cat $file/$project.time | head -2 | tail -1)
    btime=$(cat $file/$project.time | tail -1)
    echo "$project,$atime,$btime"
done
