#!/usr/bin/env bash

path_to_vt=${1}
cd "$path_to_vt" || exit 1

for sub_dir in "src" "tests/unit" "tests/perf" "tutorial" "examples"
do
  "$path_to_vt/scripts/add-license-perl.pl" "$path_to_vt/$sub_dir" "$path_to_vt/scripts/license-template"
done

result=$(git diff --name-only)

if [ -n "$result" ]; then
  echo -e "Following files have incorrect license!\n"
  echo "$result"
  exit 1
fi
