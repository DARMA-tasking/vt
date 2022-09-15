#!/usr/bin/env bash

set -xo pipefail

path_to_vt_build_dir=${1}
path_to_vt_src_dir=${2}
cd "$path_to_vt_build_dir" || exit 1

function run_schema_validator() {
    file=$1
    echo "Running schema validator on: $file"
    if python3 "${path_to_vt_src_dir}/scripts/JSON_data_files_validator.py" --file_path="$file"
    then
        echo "Valid file"
    else
        >&2 echo "Invalid schema in $file.. exiting"
        exit 1;
    fi
}

find . -iname "*.json" | grep -v "compile_commands" | while read f
do
    run_schema_validator "$f"
done

find . -iname "*.json.br" | while read f
do
    run_schema_validator "$f"
done
