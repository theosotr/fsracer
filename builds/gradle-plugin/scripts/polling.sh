#! /bin/bash

# We are polling in the `build-result.txt` that shows the result of the build.
# If this file is present, we terminate the process traced by strace,
# and exit the script.
while true; do
  if [ -f build-result.txt ]; then
    break
  fi
  sleep 10
done

exit 0
