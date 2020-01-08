#! /bin/bash

for file in $1/*; do
    project=$(basename $file)

    ./count_bugs.py $file/faults |
    while IFS= read -r line; do

        min=$(echo $line | cut -d ' ' -f1)
        mout=$(echo $line | cut -d ' ' -f2)
        ov=$(echo $line | cut -d ' ' -f3)
        echo "$project,$min,$mout,$ov"
    done
done
