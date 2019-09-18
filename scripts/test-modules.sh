#! /bin/bash

modules=$(realpath $1)


function enable_async_hooks()
{
  # Enable the async hooks by adding the necessary code at the beginning
  # of the file that corresponds to the entry point of the package.
  local preamble="const ah = require('async_hooks');\n \
    function ahf() {  }\n \
    ah.createHook({ ahf, ahf, ahf, ahf, ahf }).enable();\n";
  local code="$preamble"
  if [ -f index.js ];
  then
    sed -i "1s/^/${code}/" index.js
    return 0
  else
    main=$(cat package.json | jq -r '.main')
    if [ ! -z $main ];
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
  # Replace the test command so that we run all tests sequenatially.
  # TODO: Support more testing frameworks.
  local tcmd=$(cat package.json | jq -r '.scripts.test')
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
  elif [[ $tcmd == *"mocha "* ]];
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
    repo=$(echo "$metadata" | jq -r ".links.repository")
    echo "Cloning $module..."
    git clone $repo $module > /dev/null 2>&1
    cd $module

    enable_async_hooks
    if [ $? -ne 0 ];
    then
      # We were not able to find the entry point of the package.
      echo "$module: Unable to find its entry point" >> ../warnings.txt
      cd ..
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
      continue
    fi
    eval "timeout -s KILL 2m npm test -- $opts"
    echo "$module" >> ../success.txt
    cd ..
  fi
done < $modules

exit 0
