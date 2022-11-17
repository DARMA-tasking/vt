#!/usr/bin/env bash

root_dir=$1
bin=$2

for dir in $root_dir/challenging_*
do
    if [ -d "$dir" ]
    then
        result=$($bin $dir/*json | grep 'Result:')
        problem=$(basename $dir)
        echo "Problem: $problem; $result"
    fi
done
