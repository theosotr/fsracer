#! /bin/bash

while getopts "f:t:o:" opt; do
  case "$opt" in
    t)  trace_dir=$(realpath $OPTARG)
        ;;
    o)  output_dir=$(realpath $OPTARG)
        ;;
    f)  fsracer_dir=$(realpath $OPTARG)
        ;;
  esac
done
shift $(($OPTIND - 1));


if [ -z $fsracer_dir ]; then
  echo "You have to specify the directory of fsracer"
fi

if [ -z $trace_dir ]; then
  echo "You have to specify the directory of traces"
fi

if [ -z $output_dir ]; then
  echo "You have to specify the output directory"
fi

logs=$(echo '{}')
function handle_sigint()
{
  echo "$logs" > logs.json
  exit 0
}
trap handle_sigint SIGINT


timeout_code=137
for module_dir in $trace_dir/*; do
  module=$(basename $module_dir)
  echo "Analyzing module $module..."

  logs=$(echo "$logs" | jq ". + {\"$module\": {}}")
  logs=$(echo "$logs" | jq ". * {\"$module\": {\"traces\": []}}")
  mkdir -p $output_dir/$module

  for trace_file in $module_dir/*; do
    out=$(timeout 5m $fsracer_dir/fsracer/fsracer \
      -i $trace_file --fault-detector race 2>&1)
    rc=$?
    if [ $rc -eq 0 ]; then
      success="true"
    elif [ $rc -eq 1 ]; then
      success="false"
    else
      success="timed out"
    fi
    bt=$(basename $trace_file)
    logs=$(echo "$logs" |
    jq ".\"$module\".\"traces\" += [{\"file\": \"$bt\", \"success\": \"$success\"}]")
    echo "$out" > $output_dir/$module/$bt.out
  done
done

echo "$logs" > logs.json
exit 0
