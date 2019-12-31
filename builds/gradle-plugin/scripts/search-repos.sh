#! /bin/bash


start=0
limit=1000
API_ENDPOINT=https://api.github.com/search/repositories
while getopts "t:s:l:u:p:" opt; do
  case "$opt" in
    u)  username=$OPTARG
        ;;
    p)  password=$OPTARG
        ;;
    s)  start=$OPTARG
        ;;
    l)  limit=$OPTARG
        ;;
    t)  term=$OPTARG
        ;;
  esac
done
shift $(($OPTIND - 1));

size=$start
page=1
while [ $size -lt $limit ]; do
  curl --user "$username:$password"  \
    "$API_ENDPOINT?$term&sort=starts&order=desc&per_page=50&page=$page" |
  jq ".items[] | .clone_url" |
  cut -d "\"" -f 2
  size=$((size + 50))
  page=$((page + 1))
done
