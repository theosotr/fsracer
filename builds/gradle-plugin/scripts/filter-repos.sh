#! /bin/bash


API_ENDPOINT=https://api.github.com/repos
while getopts "u:p:" opt; do
  case "$opt" in
    u)  username=$OPTARG
        ;;
    p)  password=$OPTARG
        ;;
  esac
done
shift $(($OPTIND - 1));


for project in $(cat $1); do
  owner=$(echo $project | sed -r 's/https:\/\/github.com\/(.*)\/.*\.git/\1/g')
  repo=$(echo $project | sed -r 's/https:\/\/github.com\/.*\/(.*)\.git/\1/g')

  scode=$(curl -s -o /dev/null -w "%{http_code}" -u "$username:$password" \
    "$API_ENDPOINT/$owner/$repo/contents/gradlew")
  if [ $scode -eq 200 ]; then
    echo $project
  fi

done
