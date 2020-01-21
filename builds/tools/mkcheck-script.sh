#! /bin/bash

project=$1
output_dir=$(realpath $2)
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

echo "
filter_in:
        - Makefile.*
        - /usr/.*
        - /etc/.*
        - //.*
        - /lib/.*
        - /bin/.*
        - /.*/debian/.*
" > filter.yaml
echo "Building project with mkcheck..."
START_TIME=$(date +%s.%N)
fuzz_test --graph-path=foo.json build 2> $project_out/$project.mkerr
if [ $? -ne 0 ]; then
  exit 1
fi
ELAPSED_TIME=$(echo "$(date +%s.%N) - $START_TIME" | bc)
printf "%.2f\n" $ELAPSED_TIME > $project_out/$project.time

cp foo.json $project_out/$project.json

echo "Fuzz testing..."
START_TIME=$(date +%s.%N)
fuzz_test --graph-path=foo.json \
  --rule-path filter.yaml fuzz \
  > $project_out/$project.fuzz 2> $project_out/$project.fuzzerr
if [ $? -ne 0 ]; then
  exit 1
fi
ELAPSED_TIME=$(echo "$(date +%s.%N) - $START_TIME" | bc)
printf "%.2f\n" $ELAPSED_TIME >> $project_out/$project.time

echo "Race testing..."
START_TIME=$(date +%s.%N)
fuzz_test --graph-path=foo.json \
  --rule-path filter.yaml race \
  > $project_out/$project.race 2> $project_out/$project.racerr

if [ $? -ne 0 ]; then
  exit 1
fi
ELAPSED_TIME=$(echo "$(date +%s.%N) - $START_TIME" | bc)
printf "%.2f\n" $ELAPSED_TIME >> $project_out/$project.time
