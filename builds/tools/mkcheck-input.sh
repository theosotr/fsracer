#! /bin/bash

mkdir -p inputs
for file in /mkcheck/out/out/*; do
    project=$(basename $file)
    if [ -f $file/$project.fuzz ]; then
        ./analyze-fuzz.py $file/$project.fuzz |
        sed -r "s/(\/build\/$project)-[0-9a-zA-Z]+(\/.*)/\1\2/g" > inputs/$project.mkcheck
    fi
done
