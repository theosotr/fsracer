#! /bin/bash


run=0
install=0
while getopts "m:d:f:o:ri" opt; do
  case "$opt" in
    m)  modules=$OPTARG
        ;;
    d)  dynamo_dir=$(realpath $OPTARG)
        ;;
    f)  fsracer_dir=$(realpath $OPTARG)
        ;;
    o)  output_dir=$(realpath $OPTARG)
        ;;
    r) run=1
        ;;
    i) install=1
        ;;
  esac
done
shift $(($OPTIND - 1));


if [ -z  $modules ]; then
  echo "You have to specify the path to the modules file (option -m)"
  exit 1
fi

if [ ! -z  $dynamo_dir ]; then
  if [ -z  $fsracer_dir ]; then
    echo "The -d option requires the path to the FSracer targets (option -f)"
    exit 1
  fi
fi


if [ -z  $modules ]; then
  echo "You have to specify the path to output directory (option -o)"
  exit 1
fi


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


function init_test_cmds()
{
  json=$1
  module=$2

  echo "$json" |
  jq ". * {\"$module\": {\"test-runs\": []}}"
}


function add_to_test_cmds()
{
  json=$1
  module=$2
  cmd=$3
  success=$4

  echo "$json" |
  jq ".\"$module\".\"test-runs\" += [{\"cmd\": \"$cmd\", \"success\": $success}]"
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
  local module=$(basename $(pwd))

  # Try the first case first.
  # Check whether there exist a file named 'index.js'
  rc=1
  for main in $(find . -type f -name 'index.js'); do
    rc=0
    if ! grep -qoP "const ah = require\('async_hooks'\)" $main; then
      sed -i "1s/^/${code}/" $main
    fi
    logs=$(add_key "$logs" "$module" "entry" "$main")
  done
  if [ $rc -eq 1 ]; then
    # If not any file named 'index.js' is found, then check
    # the property 'main' found in the package.json file.
    # This property indicates the entry point of the package.
    main=$(cat package.json | jq -r '.main')
    if [ ! -z $main  ]; then
      # The entry point was found, so add the preable code.
      if ! grep -qoP "const ah = require\('async_hooks'\)" $main; then
        sed -i "1s/^/${code}/" $main
      fi
      logs=$(add_key "$logs" "$module" "entry" "$main")
      rc=0
    fi
  fi
  return $rc
}


function run_tests()
{
  local cmd test_cmd
  test_cmd=$1

  if [ ! -z $dynamo_dir ]; then
    cmd="$dynamo_dir/bin64/drrun \
      -c $fsracer_dir/drfsracer/libdrfsracer.so \
      -g node \
      --output-trace $output_dir/$module/$module.trace"
    cmd="$cmd -- $test_cmd"
  else
    cmd="$test_cmd"
  fi
  cmd="timeout 10m $cmd"

  module=$(basename $(pwd))
  pre_out=$(find $output_dir/$module)
  if [ $run -eq 0 ]; then
    logs=$(add_to_test_cmds "$logs" "$module" "$cmd" true)
    return
  fi
  out=$(eval "$cmd" 2>&1)
  rc=$?
  success=true

  if [ ! -z $dynamo_dir ]; then
    if [[ "$pre_out" == "$(find $output_dir/$module)"
        || "$out" =~ "Runtime Error" || "$out" =~ "SEGV" ]]; then
      echo "$out" >> $output_dir/$module/$module.err
      success=false
    fi
  else
    if [ $rc -ne 0 ]; then
      echo "$out" >> $output_dir/$module/$module.test.err
      success=false
    fi
  fi
  logs=$(add_to_test_cmds "$logs" "$module" "$cmd" $success)
}


function prepare_and_run_tests()
{
  local test_cmd framework
  test_cmd=$1
  framework=$2

  module=$(basename $(pwd))

  logs=$(add_key "$logs" "$module" "framework" "$framework")
  logs=$(add_key "$logs" "$module" "test-cmd" "$test_cmd")
  logs=$(init_test_cmds "$logs" "$module")
  run_tests "$test_cmd"
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
  local tcmd test_cmd module
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
    logs=$(init_test_cmds "$logs" "$module")
    for f in $test_files; do
      run_tests "node ./node_modules/tap/bin/run.js $f -j 1"
    done
  elif [[ $tcmd == *"ava"* ]]; then
    test_cmd=$(get_ava_cmd "$tcmd")
    prepare_and_run_tests "$test_cmd" "ava"
  elif [[ $tcmd == *"jest"* ]]; then
    test_cmd=$(get_jest_cmd "$tcmd")
    prepare_and_run_tests "$test_cmd" "jest"
  elif [[ $tcmd == *"mocha"* ]]; then
    test_cmd=$(get_mocha_cmd "$tcmd")
    prepare_and_run_tests "$test_cmd" "mocha"
  else
    logs=$(add_key "$logs" "$module" "error" "Unknown testing framework")
  fi
}


function clear_repo()
{
  cd ..
  rm -rf $1
}


function clone_module()
{
  local module repo
  module=$1
  repo=$2

  if [ -d "$module" ]; then
    return 0
  fi

  if [ "$repo" != "null" ]; then
    # Cloning repo with git
    git clone "$repo" "$module" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
      return 1
    fi
  else
    # Get the source code from the npm registry.
    npm v $module dist.tarball |
    xargs curl -s |
    tar -xz > /dev/null

    if [ $? -ne 0 ]; then
      return 1
    fi

    mv package $module
  fi
  return 0
}


function install_module()
{
  local module
  module=$1

  if [ -d node_modules ]; then
    # Heuristic: if a directory named 'node_modules' exist, we presume
    # that we have already installed the npm package.
    return 0
  fi

  enable_async_hooks
  if [ $? -ne 0 ]; then
    # We were not able to find the entry point of the package.
    logs=$(add_key "$logs" "$module" "error" "Unable to find entry point")
    return 1
  fi

  if [ ! -f package-lock.json ]; then
    npm i --package-lock-only > /dev/null 2>&1
  fi
  npm audit fix --force > /dev/null 2>&1
  npm install > /dev/null 2>&1
  return 0
}


function fetch_module()
{
  local module
  module=$1
  if [ -d "$module" ]; then
    return 0
  else
    metadata=$(curl -s -X GET "https://api.npms.io/v2/package/$module" |
    jq -r '.collected.metadata')

    logs=$(echo "$logs" | jq ". + {\"$module\": {}}")
    if echo "$metadata" | jq -e 'has("deprecated")' > /dev/null; then
      logs=$(add_key "$logs" "$module" "deprecated" "true")
      return 1
    fi

    if [ "true" = $(echo "$metadata" | jq -r ".hasTestScript") ]; then
      repo=$(echo "$metadata" | jq -r ".links.repository")

      clone_module "$module" "$repo"
      if [ $? -ne 0 ]; then
        logs=$(add_key "$logs" "$module" "error" "Unable to clone")
        return 1
      fi
    else
      logs=$(add_key "$logs" "$module" "test-script" "false")
      return 1
    fi
  fi
  return 0
}


for module in $(cat $modules);
do
  echo "Processing $module..."
  fetch_module "$module"

  if [ $? -ne 0 ]; then
    continue
  fi

  cd $module
  if [ ! -f package.json ]; then
    # We are unable to find the package.json file.
    logs=$(add_key "$logs" "$module" "error" "Unable to find package.json")
    return 1
  fi
  if [ $install -eq 1 ]; then
    echo "Installing $module..."
    install_module "$module"
    if [ $? -ne 0 ]; then
      cd ..
      continue
    fi
  fi
  echo "Testing $module..."
  mkdir -p $output_dir/$module
  call_tests
  if [ -z "$(ls $output_dir/$module)" ]; then
    # The directory of traces is empty; so remove it.
    rm -r $output_dir/$module
  fi
  cd ..
done

echo "$logs" > logs.json
exit 0
