#! /bin/bash

term=$1
limit=5000
start=0
size=250

while [ $start -le $limit ];
do
  curl -s -X GET "https://api.npms.io/v2/search?q=$term+not:deprecated&from=$start&size=$size" |
  jq '.results[] | .package.name'
  start=$((start + $size))
done
