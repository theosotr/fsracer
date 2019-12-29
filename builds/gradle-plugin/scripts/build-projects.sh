#! /bin/bash

HOME=/home/fsmove

with_strace=0
while getopts "p:o:s" opt; do
  case "$opt" in
    p)  projects=$(realpath $OPTARG)
        ;;
    o)  output_dir=$(realpath $OPTARG)
        ;;
    s)  with_strace=1
        ;;
  esac
done
shift $(($OPTIND - 1));


mkdir -p $output_dir/out
mkdir -p $output_dir/tmp

for project in $(cat $projects); do
  if [ -d $output_dir/out/$project ]; then
    echo "Skipping $project..."
    continue
  fi
  if echo $project | grep -q -oP '^https://'; then
    pname=$(echo $project | sed -r 's/^https:\/\/github.com\/.*\/(.*)\.git/\1/g')
  else
    pname=$project
  fi
  docker_cmd="$HOME/plugin/scripts/build-project.sh $project $HOME/out $with_strace"
  sudo docker run --name $pname \
    -v $output_dir/tmp:$HOME/out \
    --cap-add=SYS_PTRACE \
    --rm \
    --security-opt seccomp:unconfined gradle-image /bin/bash -c "$docker_cmd"
  if [ $? -ne 0 ]; then
    sudo rm -rf $output_dir/tmp/*
    continue
  fi
  mv $output_dir/tmp/$pname $output_dir/out
done
