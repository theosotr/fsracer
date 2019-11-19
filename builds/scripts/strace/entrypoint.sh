#!/usr/bin/env bash

if [ -z "$1" ]
  then
    echo "No source name supplied"
    exit -1
fi

source=$1

mkdir -p /results/$source
sbuild --apt-update --no-apt-upgrade --no-apt-distupgrade --batch \
    --stats-dir=/var/log/sbuild/stats --dist=stable --arch=amd64 $source
for f in $(ls /results/$source/); do
    if [ ! -s "/results/$source/$f" ]; then
        rm -f /results/$source/$f
    fi
done
