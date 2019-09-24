#! /bin/bash

HOME=/home/fsracer
output_dir=$(realpath $1)
modules=$(realpath $2)
dynamo_dir=$HOME/dynamo
fsracer_dir=$HOME/fsracer/build/tools

mkdir -p $output_dir

sudo docker run \
  -v $output_dir:$HOME/out \
  -v $modules:$HOME/modules.txt fsracer /bin/bash \
  -c "$HOME/fsracer/scripts/collect-traces.sh \
  -m $HOME/modules.txt \
  -d $dynamo_dir \
  -f $fsracer_dir \
  -o $HOME/out"
