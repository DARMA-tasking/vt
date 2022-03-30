#!/usr/bin/env bash

path_to_vt_build_dir=${1}
cd "$path_to_vt_build_dir" || exit 1

#git clone -b "develop" --depth 1 https://github.com/DARMA-tasking/LB-analysis-framework.git LBAF

function run_schema_validator() {
    file=$1
    echo "Running schema validator on: $file"
    #./LBAF/
}

for i in $(find . -iname "*.json" | grep -v "compile_commands")
do
    run_schema_validator "$i"
done

for i in $(find . -iname "*.json.br")
do
    run_schema_validator "$i"
done
