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
  docker_cmd="/root/plugin/scripts/build-project.sh $project /root/out"
  sudo docker run --name $project \
    -v $output_dir:/root/out \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp:unconfined gradle-image /bin/bash -c "$docker_cmd"
  sudo docker cp $project:/root/$project/graph.json $output_dir/$project
  sudo docker rm $project
done
