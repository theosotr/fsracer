#! /bin/bash

if [ ! -d .env ];
then
  virtualenv -p python3 .env
fi

source .env/bin/activate
pip install -r requirements.txt
# Required for parsing JSON in bash scripts
sudo apt install jq
