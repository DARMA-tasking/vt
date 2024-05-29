#!/usr/bin/env bash

path_to_vt=${1}

cd "$path_to_vt" || exit 1

for sub_dir in "src" "tests"
do
  python3 "${path_to_vt}/scripts/generate_header_guards_and_license.py" -s=${sub_dir} -l="${path_to_vt}/scripts/license-template"
done

# Check for modified files
modified_files=$(git ls-files -m)

if [ -n "$modified_files" ]; then
  echo "The following files have been modified:"
  echo "$modified_files"
else
  echo "No files have been modified."
fi
