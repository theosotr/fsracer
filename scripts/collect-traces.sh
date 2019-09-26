#! /bin/bash


logs=$(echo '{}')
function add_key()
{
  json=$1
  module=$2
  k=$3
  v=$4

  echo "$json" |
  jq ". * {\"$module\": {\"$k\": \"$v\"}}"
}


function init_dynamorio_cmds()
{
  json=$1
  module=$2

  echo "$json" |
  jq ". * {\"$module\": {\"dynamorio\": []}}"
}


function add_to_dynamorio_cmds()
{
  json=$1
  module=$2
  cmd=$3
  success=$4

  echo "$json" |
  jq ".\"$module\".dynamorio += [{\"cmd\": \"$cmd\", \"success\": $success}]"
}


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
  module=$(basename $(pwd))

  # Try the first case first.
  # Check whether there exist a file named 'index.js'
  rc=1
  for main in $(find . -type f -name 'index.js'); do
    rc=0
    sed -i "1s/^/${code}/" $main
    logs=$(add_key "$logs" "$module" "entry" "$main")
  done
  if [ $rc -eq 1 ]; then
    # If not any file named 'index.js' is found, then check
    # the property 'main' found in the package.json file.
    # This property indicates the entry point of the package.
    main=$(cat package.json | jq -r '.main')
    if [ ! -z $main  ]; then
      # The entry point was found, so add the preable code.
      sed -i "1s/^/${code}/" $main
      logs=$(add_key "$logs" "$module" "entry" "$main")
      rc=0
    fi
  fi
  return $rc
}


function execute_dynamo()
{
  local base_cmd test_cmd test_file
  base_cmd=$1
  test_cmd=$2
  test_file=$3

  cmd="$base_cmd -- $test_cmd"

  if [ ! -z $test_file ]; then
    cmd="$cmd $test_file"
  fi
  echo "Invoking the command: $cmd"

  module=$(basename $(pwd))
  pre_out=$(find $output_dir/$module)
  out=$(eval "$cmd" 2>&1)
  success=true
  rc=0
  if [[ "$pre_out" == "$(find $output_dir/$module)"
      || "$out" =~ "Runtime Error" || "$out" =~ "SEGV" ]]; then
    echo "$out" >> $output_dir/$module/$module.err
    success=false
    rc=1
  fi
  logs=$(add_to_dynamorio_cmds "$logs" "$module" "$cmd" $success)
  return $rc
}


function run_tests_with_dynamorio()
{
  local base_cmd dynamo_cmd test_cmd framework
  base_cmd=$1
  test_cmd=$2
  framework=$3

  module=$(basename $(pwd))

  logs=$(add_key "$logs" "$module" "framework" "$framework")
  logs=$(add_key "$logs" "$module" "test-cmd" "$test_cmd")
  logs=$(init_dynamorio_cmds "$logs" "$module")
  execute_dynamo "$base_cmd" "$test_cmd"
}


function add_cli_option()
{
  local tcmd
  tcmd=$1
  if ! echo "$tcmd" | grep -qoP ".*$2.*"; then
    echo "$tcmd $2"
  else
    echo "$tcmd"
  fi
}


function get_ava_cmd()
{
  local tcmd
  tcmd=$1


  if echo "$tcmd" | grep -qoP ".*\/ava\/cli.js"; then
    if ! echo "$tcmd" | grep -qoP "^node[ ].*"; then
      tcmd="node $tcmd"
    fi
  else
    tcmd=$(echo "$tcmd" | sed 's/ava/node .\/node_modules\/ava\/cli.js/g')
  fi

  add_cli_option "$tcmd" "--serial" |
  sed -r 's/.*(&&[ ])?(node[ ][^&]+)([ ]&&)?.*/\2/g'
}


function get_jest_cmd()
{
  local tcmd
  tcmd=$1
  if echo "$tcmd" | grep -qoP ".*\/jest\/bin"; then
    if ! echo "$tcmd" | grep -qoP "^node[ ].*"; then
      tcmd="node $tcmd"
    fi
  else
    tcmd=$(echo "$tcmd" | sed 's/jest/node .\/node_modules\/jest\/bin\/jest.js/g')
  fi
  tcmd=$(add_cli_option "$tcmd" "--runInBand")
  add_cli_option "$tcmd" "--detectOpenHandles" |
  sed -r 's/.*(&&[ ])?(node[ ][^&]+)([ ]&&)?.*/\2/g'
}


function get_mocha_cmd()
{
  local tcmd
  tcmd=$1

  # Some preprocessing first: Do some necessary replacements.
  tcmd=$(echo "$tcmd" |
  sed 's/--async-only//g' |
  sed 's/_mocha/mocha/g' |
  sed 's/.bin\/mocha/mocha\/bin\/mocha/g' |
  sed -r 's/--timeout[ ][0-9]+s?//g; s/-t[ ][0-9]+s?//g')
  if echo "$tcmd" | grep -qoP ".*\/mocha\/bin\/mocha"; then
    # Maybe the test script uses the path to the mocha script.
    if ! echo $tcmd | grep -qoP "^node[ ].*"; then
      # However, it does not use the node executable explicitly.
      tcmd="node $tcmd"
    fi
  else
    # The test script simply uses "mocha".
    tcmd=$(echo "$tcmd" | sed 's/mocha/node .\/node_modules\/mocha\/bin\/mocha/g')
  fi
  echo "$tcmd" |
  sed -r 's/.*(&&[ ])?(node[ ][^&]+)([ ]&&)?.*/\2/g'
}


function call_tests()
{
  # TODO: Support more testing frameworks.
  # This function returns the necessary options (based on the supported
  # testing framework) to run tests sequentially.

  # FIXME: This is a hack.
  # Modify test script and package.json to ignore all linters.
  local tcmd test_cmd base_cmd module
  base_cmd=$1
  module="$(basename $(pwd))"

  tcmd=$(cat package.json |
  jq -r '.scripts.test' |
  sed 's/\(&&\)\?[ ]\?xo[^&]*&&[ ]\?//g' |
  sed 's/[ ]\?\(&&\)\?[ ]\?tsd[^&]*//g' |
  sed 's/\(&&\)\?[ ]\?standard[^&]*&&[ ]\?//g' |
  sed 's/nyc//g; s/npx//g; s/"/\\"/g')

  logs=$(add_key "$logs" "$module" "original-test-cmd" "$tcmd")
  if [[ $tcmd == *"tap"* ]]; then
    logs=$(add_key "$logs" "$module" "framework" "tap")

    test_files=$(echo "$tcmd" |
    grep -oP "tap([ ][a-zA-Z0-9\.\/\*]+)?$" |
    sed 's/tap//g' |
    xargs)
    if [ -z "$test_files" ]; then
      test_files="test/*.js"
    fi

    if [ -d "$test_files" ]; then
      # Create a glob pattern.
      test_files="$test_files/*.js"
    fi
    logs=$(add_key "$logs" "$module" "test-cmd" "node $test_files")
    logs=$(init_dynamorio_cmds "$logs" "$module")
    for f in $test_files; do
      execute_dynamo "$base_cmd" "node" "$f"
    done
  elif [[ $tcmd == *"ava"* ]]; then
    test_cmd=$(get_ava_cmd "$tcmd")
    run_tests_with_dynamorio "$base_cmd" "$test_cmd" "ava"
  elif [[ $tcmd == *"jest"* ]]; then
    test_cmd=$(get_jest_cmd "$tcmd")
    run_tests_with_dynamorio "$base_cmd" "$test_cmd" "jest"
  elif [[ $tcmd == *"mocha"* ]]; then
    test_cmd=$(get_mocha_cmd "$tcmd")
    run_tests_with_dynamorio "$base_cmd" "$test_cmd" "mocha"
  else
    logs=$(add_key "$logs" "$module" "error" "Unknown testing framework")
  fi
}


function clear_repo()
{
  cd ..
  rm -rf $1
}


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


if [ -z  $modules ]; then
  echo "You have to specify the path to the modules file (option -m)"
  exit 1
fi

if [ -z  $dynamo_dir ]; then
  echo "You have to specify the path to the DynamoRIO installation (option -d)"
  exit 1
fi

if [ -z  $fsracer_dir ]; then
  echo "You have to specify the path to the FSracer targets (option -f)"
  exit 1
fi

if [ -z  $modules ]; then
  echo "You have to specify the path to output directory (option -o)"
  exit 1
fi

for module in $(cat $modules);
do
  echo "Processing $module..."
  metadata=$(curl -s -X GET "https://api.npms.io/v2/package/$module" |
  jq -r '.collected.metadata')

  logs=$(echo "$logs" | jq ". + {\"$module\": {}}")
  if echo "$metadata" | jq -e 'has("deprecated")' > /dev/null; then
    logs=$(add_key "$logs" "$module" "deprecated" "true")
    continue
  fi

  if [ "true" = $(echo "$metadata" | jq -r ".hasTestScript") ]; then
    if [ -d "$module" ]; then
      # The module has already been analyzed.
      logs=$(add_key "$logs" "$module" "skipped" "true")
      continue
    fi
    repo=$(echo "$metadata" | jq -r ".links.repository")

    echo "Cloning $module..."
    if [ "$repo" != "null" ]; then
      # Cloning repo with git
      git clone "$repo" "$module" > /dev/null
      if [ $? -ne 0 ]; then
        logs=$(add_key "$logs" "$module" "error" "Unable to clone")
        continue
      fi
    else
      # Get the source code from the npm registry.
      npm v $module dist.tarball |
      xargs curl -s |
      tar -xz > /dev/null

      if [ $? -ne 0 ]; then
        logs=$(add_key "$logs" "$module" "error" "Unable to extract")
        continue
      fi

      mv package $module
    fi
    cd $module

    if [ ! -f package.json ]; then
      # We are unable to find the package.json file.
      logs=$(add_key "$logs" "$module" "error" "Unable to find package.json")
      clear_repo $module
      continue
    fi

    enable_async_hooks
    if [ $? -ne 0 ]; then
      # We were not able to find the entry point of the package.
      logs=$(add_key "$logs" "$module" "error" "Unable to find entry point")
      clear_repo $module
      continue
    fi

    echo "Installing $module..."
    if [ ! -f package-lock.json ]; then
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
    if [ -z "$(ls $output_dir/$module)" ]; then
      # The directory of traces is empty; so remove it.
      rm -r $output_dir/$module
    fi
    clear_repo $module
  else
    logs=$(add_key "$logs" "$module" "test-script" "false")
  fi
done

echo "$logs" > logs.json
exit 0
