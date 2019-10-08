#! /bin/bash


# Look for parallel tasks
# Look for daemon
# Find gradlew
# Add plugin
# Execute

plugin=/root/plugin/build/libs/plugin.jar
project=$1
output_dir=$(realpath $2)

project_out=$output_dir/$project
mkdir -p $project_out

project_repo=$(python3 /root/plugin/scripts/collect-gradle-repos.py "$project")

if [ $? -ne 0 ]; then
  echo "Repo not found" > $project_out/err
  exit 1
fi
echo "Project repo $project_repo"

git clone "$project_repo" /root/$project
if [ $? -ne 0 ]; then
  echo "Unable to clone" > $project_out/err
  exit 1
fi
cd $project

# Disable gradle daemon and parallel execution
find . -name 'gradle.properties' |
xargs -i sed -i 's/org\.gradle\.parallel=true/org\.gradle\.parallel=false/g' {}

code="apply plugin: 'org.fsracer.gradle.fsracer-plugin'\nbuildscript { dependencies { classpath files('$plugin') } }\n"
# Heuristic: Search for file whose name is bu
find . -regex '.*build.gradle.*' -type f -printf "%d %p\n" |
sort -n |
head -1 |
cut -d' ' -f2 |
xargs -i sed -i "1s;^;${code};" {}

if [ $? -ne 0 ]; then
  echo "Unable to find build.gradle file" > $project_out/err
fi

gradlew=$(find . -name 'gradlew' -type f -printf "%d %p\n" |
sort -n |
head -1 |
cut -d' ' -f2)

if [ $? -ne 0 ]; then
  echo "Unable to find gradlew file" > $project_out/err
fi

echo $gradlew

# When we trace gradle builds, strace might hang because it's waiting for
# the gradle daemon to exit which is spanwed by the gradlew script.
# So we run the build script in the background and we do some kind of polling
# in order to know when the build actually finishes.
eval "timeout -s KILL 30m strace \
  -s 300 \
  -o $project_out/$project.strace \
  -e \"$(tr -s '\r\n' ',' < /root/syscalls.txt | sed -e 's/,$/\n/')\" \
  -f $gradlew build --no-parallel --daemon &"
pid=$!

# We are polling in the `build-result.txt` that shows the result of the build.
# If this file is present, we terminate the process traced by strace,
# and exit the script.
while true; do
  if [ -f build-result.txt ]; then
    kill -s KILL $pid
    break
  fi
  sleep 10
done

exit 0
