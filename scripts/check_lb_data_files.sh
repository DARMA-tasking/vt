#!/usr/bin/env bash

set -exo pipefail

path_to_vt_build_dir=${1}
cd "$path_to_vt_build_dir" || exit 1

if [ ! -d ../LBAF ]
then
    git clone -b "221-prepare-a-standalone-script-for-validating-schema-of-JSON-data-files" --depth 1 https://github.com/DARMA-tasking/LB-analysis-framework.git LBAF
    mv LBAF ../
fi

function run_schema_validator() {
    file=$1
    echo "Running schema validator on: $file"
    if python3 ../LBAF/src/Utils/JSON_data_files_validator.py --file_path="$file"
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
