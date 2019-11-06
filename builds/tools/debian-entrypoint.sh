#!/usr/bin/env bash

if [ -z "$1" ]
  then
    echo "No source name supplied"
    exit -1
fi

source=$1

mkdir -p /var/log/sbuild/straces/$source
mkdir -p /var/log/sbuild/straces/temp
ls /var/log/sbuild/straces/
sbuild --apt-update --no-apt-upgrade --no-apt-distupgrade --batch \
    --stats-dir=/var/log/sbuild/stats --dist=stable --arch=amd64 $source
for f in $(ls /var/log/sbuild/straces/temp/); do
    if [ -s "/var/log/sbuild/straces/temp/$f" ]; then
        cp /var/log/sbuild/straces/temp/$f /var/log/sbuild/straces/$source/$source.$f
    fi
done
rm -rf /var/log/sbuild/straces/temp
