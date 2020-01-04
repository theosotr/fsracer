#! /bin/bash

mkdir -p inputs
for file in $1/*; do
    project=$(basename $file)
    if [ -f $file/faults ]; then
        ./analyze-fsmove.py $file/faults |
        sed -r "s/(\/build\/$project)-[0-9a-zA-Z]+(\/.*)/\1\2/g" > inputs/$project.fsmove
    fi
done
