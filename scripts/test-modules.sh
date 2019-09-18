#! /bin/bash

modules=$(realpath $1)


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


function get_test_options()
{
  # TODO: Support more testing frameworks.
  # This function returns the necessary options (based on the supported
  # testing framework) to run tests sequentially.

  # FIXME: This is a hack.
  # Modify test script and package.json to ignore all linters.
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
    echo "-j 1"
    return 0
  elif [[ $tcmd == *"ava"* ]];
  then
    echo "--serial"
    return 0
  elif [[ $tcmd == *"jest"* ]];
  then
    echo "--runInBand --detectOpenHandles"
    return 0
  elif [[ $tcmd == *"mocha"* ]];
  then
    return 0
  else
    return 1;
  fi
}


cp /dev/null warnings.txt
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
    git clone $repo $module > /dev/null 2>&1
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
      echo "$module: Unable to find its entry point" >> ../warnings.txt
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

    opts=$(get_test_options)
    if [ $? -ne 0 ];
    then
      echo "$module: Unable to find the testing framework" >> ../warnings.txt
      cd ..
      #rm $module -rf
      continue
    fi
    eval "timeout -s KILL 2m npm test -- $opts"
    echo "$module" >> ../success.txt
    cd ..
  fi
done < $modules

exit 0
