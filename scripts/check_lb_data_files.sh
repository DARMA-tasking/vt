#!/usr/bin/env bash

set -exo pipefail

path_to_vt_build_dir=${1}
path_to_vt_src_dir=${2}
cd "$path_to_vt_build_dir" || exit 1

function run_schema_validator() {
    file=$1
    echo ""
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

find "${path_to_vt_src_dir}/examples" -iname "*.json" | while read f
do
    run_schema_validator "$f"
done

find . -iname "*.json.br" | while read f
do
    run_schema_validator "$f"
done

# Compare output of the lb_data_file_generator example with reference file
if ! python3 "${path_to_vt_src_dir}/scripts/generate_and_validate_lb_data_file.py" \
    -f "${path_to_vt_build_dir}/examples/lb_data/lb_data_file_generator_1_LBDatafile.0.json" \
    -r "${path_to_vt_src_dir}/examples/lb_data/lb_data_file_example.json"
then
    exit 2;
fi
