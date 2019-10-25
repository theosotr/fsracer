#! /bin/bash

while getopts "p:o:" opt; do
  case "$opt" in
    p)  projects=$(realpath $OPTARG)
        ;;
    o)  output_dir=$(realpath $OPTARG)
        ;;
  esac
done
shift $(($OPTIND - 1));


for project in $(cat $projects); do
  if [ -d $output_dir/$project ]; then
    echo "Skipping $project..."
    continue
  fi
  if echo $project | grep -q -oP '^https://'; then
    pname=$(echo $project | sed -r 's/^https:\/\/github.com\/.*\/(.*)\.git/\1/g')
  else
    pname=$project
  fi
  docker_cmd="/root/plugin/scripts/build-project.sh $project /root/out"
  sudo docker run --name $pname \
    -v $output_dir:/root/out \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp:unconfined gradle-image /bin/bash -c "$docker_cmd"
  sudo docker cp $pname:/root/$pname/graph.json $output_dir/$pname
  sudo docker rm $pname
done
