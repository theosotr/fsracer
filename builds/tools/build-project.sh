#! /bin/bash

eval `opam config env`

project=$1
output_dir=$(realpath $2)
strace=$3

project_repo=$project
project=$(echo $project | sed -r 's/^https:\/\/github.com\/.*\/(.*)\.git/\1/g')

project_out=$output_dir/$project
mkdir -p $project_out

git clone "$project_repo" $HOME/$project
if [ $? -ne 0 ]; then
  echo "Unable to clone" > $project_out/err
  exit 1
fi
cd $project


if [ -f CMakeLists.txt ]; then
  exit 1
fi

if [ -f configure ]; then
  ./configure
fi

strace -s 300 -f -e "$(tr -s '\r\n' ',' < $HOME/syscalls.txt | \
  sed -e 's/,$/\n/')" -o $project_out/$project.strace fsmake-make

if [ $? -ne 0 ]; then
  exit 1
fi
make -pn > $project_out/$project.makedb
