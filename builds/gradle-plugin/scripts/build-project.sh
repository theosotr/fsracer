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
xargs -i sed -i 's/org\.gradle\.daemon=true/org\.gradle\.daemon=false/g; s/org\.gradle\.parallel=true/org\.gradle\.parallel=false/g' {}

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

eval "timeout -s KILL 30m strace -s 300 -o $project_out/$project.strace -f \
  $gradlew build --no-parallel --no-daemon"
