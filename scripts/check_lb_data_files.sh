#!/usr/bin/env bash

set -exo pipefail

path_to_vt_build_dir=${1}
cd "$path_to_vt_build_dir" || exit 1

function run_schema_validator() {
    file=$1
    echo "Running schema validator on: $file"
    if python3 JSON_data_files_validator.py --file_path="$file"
    then
        echo "Valid file"
    else
        >&2 echo "Invalid schema in $file.. exiting"
        exit 1;
    fi
}

for i in $(find . -iname "*.json" | grep -v "compile_commands")
do
    run_schema_validator "$i"
done

for i in $(find . -iname "*.json.br")
do
    run_schema_validator "$i"
done
