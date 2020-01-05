#! /bin/bash


for file in $1/*; do
    project=$(basename $file)
    atime=$(grep -oP 'Analysis time: [0-9.]+' $file/faults |
      sed -r 's/Analysis time: ([0-9.]+)/\1/g')
    btime=$(grep -oP 'Bug detection time: [0-9.]+' $file/faults |
      sed -r 's/Bug detection time: ([0-9.]+)/\1/g')
    echo "$project,$btime"
done
