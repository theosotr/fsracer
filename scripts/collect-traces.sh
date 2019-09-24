#! /bin/bash
while getopts "m:d:f:o:" opt; do
  case "$opt" in
    m)  modules=$OPTARG
        ;;
    d)  dynamo_dir=$(realpath $OPTARG)
        ;;
    f)  fsracer_dir=$(realpath $OPTARG)
        ;;
    o)  output_dir=$(realpath $OPTARG)
        ;;
  esac
done
shift $(($OPTIND - 1));


if [ -z  $modules ];
then
  echo "You have to specify the path to the modules file (option -m)"
  exit 1
fi

if [ -z  $dynamo_dir ];
then
  echo "You have to specify the path to the DynamoRIO installation (option -d)"
  exit 1
fi

if [ -z  $fsracer_dir ];
then
  echo "You have to specify the path to the FSracer targets (option -f)"
  exit 1
fi

if [ -z  $modules ];
then
  echo "You have to specify the path to output directory (option -o)"
  exit 1
fi


function enable_async_hooks()
{
  # Enable the async hooks by adding the necessary code at the beginning
  # of the file that corresponds to the entry point of the package.
  local preamble="const ah = require('async_hooks');\n \
    function _init_() {}\n \
    function _before_() {}\n \
    function _after_() {}\n \
    function _destroy_() {}\n \
    function _presolve_() {}\n \
    ah.createHook({_init_, _before_, _after_, _destroy_, _presolve_}).enable();\n";
  local code="$preamble"
  if [ -f index.js ];
  then
    sed -i "1s/^/${code}/" index.js
    return 0
  else
    main=$(cat package.json | jq -r '.main')
    if [ -z $main ];
    then
      return 1
    fi
    sed -i "1s/^/${code}/" $main
    return 0
  fi
  return 1
}


function execute_dynamo()
{
  base_cmd=$1
  test_cmd=$2
  test_file=$3

  cmd="$base_cmd -- $test_cmd"

  if [ ! -z $test_file ];
  then
    cmd="$cmd $test_file"
  fi
  echo "Invoking the command: $cmd"

  module=$(basename $(pwd))
  pre_out=$(find $output_dir/$module)
  out=$(eval "$cmd" 2>&1)
  if [[ "$pre_out" == "$(find $output_dir/$module)"
      || "$out" =~ "Runtime Error" || "$out" =~ "SEGV" ]];
  then
    echo "$module: No trace file is generated. Command: $cmd" >> ../errors.txt
    echo "$out" >> $output_dir/$module/$module.err
    return 1
  fi
  return 0
}


function code_to_framework()
{
  case $1 in
  1) echo "tap"
     ;;
  2) echo "ava"
     ;;
  3) echo "jest"
     ;;
  4) echo "mocha"
     ;;
  esac
}


function call_tests()
{
  # TODO: Support more testing frameworks.
  # This function returns the necessary options (based on the supported
  # testing framework) to run tests sequentially.

  # FIXME: This is a hack.
  # Modify test script and package.json to ignore all linters.
  base_cmd=$1
  local tcmd=$(cat package.json |
  jq -r '.scripts.test' |
  sed 's/\(&&\)\?[ ]\?xo[^&]*&&[ ]\?//g' |
  sed 's/[ ]\?\(&&\)\?[ ]\?tsd[^&]*//g' |
  sed 's/\(&&\)\?[ ]\?standard[^&]*&&[ ]\?//g')
  # Now replace the package.json with the new test script command.
  jq -e "(.scripts.test) = \"$tcmd\"" package.json |
  jq 'del(.scripts.lint)' > tmp && mv tmp package.json

  sed -i 's/npm run lint//g;s/standard//g' package.json
  if [[ $tcmd == *"tap"* ]];
  then
    test_files=$(echo "$tcmd" |
    grep -oP "tap([ ][a-zA-Z0-9\.\/\*]+)?$" |
    sed 's/tap//g' |
    xargs)

    if [ -z "$test_files" ];
    then
      test_files="test/*.js"
    fi
    for f in test/*.js; do
      execute_dynamo "$base_cmd" "node" "$f"
    done
    return 1
  elif [[ $tcmd == *"ava"* ]];
  then
    execute_dynamo "$base_cmd" "node ./node_modules/ava/cli.js --serial"
    if [ $? -ne 0 ];
    then
      return -1
    fi
    return 2
  elif [[ $tcmd == *"jest"* ]];
  then
    execute_dynamo "$base_cmd" "node ./node_modules/jest/bin/jest.js --runInBand --detectOpenHandles"
    if [ $? -ne 0 ];
    then
      return -1
    fi
    return 3
  elif [[ $tcmd == *"mocha"* ]];
  then
    execute_dynamo "$base_cmd" "node ./node_modules/mocha/bin/mocha"
    if [ $? -ne 0 ];
    then
      return -1
    fi
    return 4
  else
    echo "$(basename $(pwd)): No supported testing framework" >> ../errors.txt
    return -1;
  fi
}


cp /dev/null errors.txt
cp /dev/null success.txt


while IFS= read module
do
  echo "Processing $module..."
  metadata=$(curl -s -X GET "https://api.npms.io/v2/package/$module" |
  jq -r '.collected.metadata')

  if echo "$metadata" | jq -e 'has("deprecated")' > /dev/null;
  then
    continue
  fi

  if [ "true" = $(echo "$metadata" | jq -r ".hasTestScript") ];
  then
    if [ -d "$module" ];
    then
      # The module has already been analyzed.
      continue
    fi
    repo=$(echo "$metadata" | jq -r ".links.repository")

    echo "Cloning $module..."
    if [ "$repo" != "null" ];
    then
      # Cloning repo with git
      git clone $repo $module > /dev/null 2>&1
    else
      # Get the source code from the npm registry.
      npm v $module dist.tarball | xargs curl -s | tar -xz > /dev/null 2>&1
      mv package $module
    fi
    cd $module

    if [ ! -f package.json ];
    then
      # We are unable to find the package.json file.
      cd ..
      rm $module -rf
      continue
    fi

    enable_async_hooks
    if [ $? -ne 0 ];
    then
      # We were not able to find the entry point of the package.
      echo "$module: Unable to find its entry point" >> ../errors.txt
      cd ..
      rm $module -rf
      continue
    fi

    echo "Installing $module..."
    if [ ! -f package-lock.json ];
    then
      npm i --package-lock-only > /dev/null 2>&1
    fi
    npm audit fix --force > /dev/null 2>&1
    npm install > /dev/null 2>&1

    echo "Testing $module..."
    mkdir -p $output_dir/$module

    # This is the base command that invokes DynamoRIO along with the
    # DRFSracer client.
    dynamo_cmd="$dynamo_dir/bin64/drrun \
      -c $fsracer_dir/drfsracer/libdrfsracer.so \
      -g node \
      --output-trace $output_dir/$module/$module.trace"
    base_cmd="timeout -s KILL 10m $dynamo_cmd"


    call_tests "$base_cmd"
    exc=$?
    if [ $exc -eq -1 ];
    then
      cd ..
      continue
    fi
    echo "$module,$(code_to_framework $exc)" >> ../success.txt
    cd ..
    rm $module -rf
  fi
done < $modules

exit 0
