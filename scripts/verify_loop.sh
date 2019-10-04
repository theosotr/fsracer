#! /bin/bash

cmd=$1
output_dir=$(realpath $2)
first_out=

module=$(basename $output_dir)


while true; do
  start=`date +%s`
  out=$(eval "timeout $cmd" 2>&1)
  end=`date +%s`
  runtime=$((end-start))

  find /tmp -newermt "-$runtime seconds" -group fsracer 2> /dev/null |
  xargs -i rm -r {} 2> /dev/null

  if [[ -z $first_out ]]; then
    first_out="$out"
  elif [ "$first_out" != "$out" ]; then
    echo "Bingo! We found a case where tests are non-deterministic"
    echo "$first_out" > $output_dir/test1.out
    echo "$out" > $output_dir/test2.out
    break
  fi
done

exit 0
