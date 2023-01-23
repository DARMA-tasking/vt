#!/usr/bin/env bash

root_dir=$1
bin=$2

function runLB() {
    dir=$1
    result=$($bin $dir/*json | grep 'Result:')
    problem=$(basename $dir)
    echo "Problem: $problem; $result"
}

for dir in $root_dir/user-defined*
do
    if [ -d "$dir" ]
    then
        runLB $dir
    fi
done

for dir in $root_dir/*subblks*
do
    if [ -d "$dir" ]
    then
        runLB $dir
    fi
done

for dir in $root_dir/challenging_*
do
    if [ -d "$dir" ]
    then
        runLB $dir
    fi
done

