#! /bin/bash


# Look for parallel tasks
# Look for daemon
# Find gradlew
# Add plugin
# Execute

modify_build_script()
{
  plugin=/root/plugin/build/libs/plugin.jar
  if [ "$1" = "groovy" ]; then
    buildscript="buildscript { dependencies { classpath files('$plugin') } }\n"
    applyplug="apply plugin: 'org.fsracer.gradle.fsracer-plugin'"
    build_file="build.gradle"
  else
    buildscript="buildscript { dependencies { classpath(files(\"$plugin\")) } }\n"
    applyplug="apply(plugin=\"org.fsracer.gradle.fsracer-plugin\")"
    build_file="build.gradle.kts"
  fi
  # Heuristic: Search for file whose name is bu
  find . -regex ".*${build_file}" -type f -printf "%d %p\n" |
  sort -n |
  head -1 |
  cut -d' ' -f2 |
  xargs -i sed -i -e "1s;^;${buildscript};" -e "\$a${applyplug}" {}
  return $?
}


project=$1
output_dir=$(realpath $2)

dir=$(dirname $0)

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

modify_build_script "groovy"
ret_groovy=$?
modify_build_script "kotlin"
ret_kotlin=$?

if [[ $ret_groovy -ne 0 && $ret_kotlin -ne 0 ]]; then
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

timeout 30m $dir/polling.sh

exit 0
