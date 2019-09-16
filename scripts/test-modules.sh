#! /bin/bash

modules=$(realpath $1)

while IFS= read module
do
  metadata=$(curl -X GET "https://api.npms.io/v2/package/$module" |
  jq -r '.collected.metadata')

  if echo "$metadata" | jq -e 'has("deprecated")' > /dev/null;
  then
    continue
  fi

  if [ "true" = $(echo "$metadata" | jq -r ".hasTestScript") ];
  then
    repo=$(echo "$metadata" | jq -r ".links.repository")
    printf "\nCloning $module...\n"
    git clone $repo $module
    cd $module

    printf "\nInstalling $module...\n"
    if [ ! -f package-lock.json ];
    then
      npm i --package-lock-only
    fi
    npm audit fix --force
    npm install

    printf "\nTesting $module...\n"
    npm test
  fi
  break
done < $modules
