#!/usr/bin/env bash

path_to_vt=${1}
wrong_files=""

cd "$path_to_vt" || exit 1

for sub_dir in "src" "tests/unit" "tests/perf"
do
  wrong_files+=$(checkguard -r "$sub_dir" -p "path -1 | prepend INCLUDED_ | upper")
done

if [[ $wrong_files ]]; then
  echo "Files with wrong header guard:"
  printf "%s\n" "$wrong_files"
  exit 1
fi
