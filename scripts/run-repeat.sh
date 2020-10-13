#!/usr/bin/env bash

if test $# -lt 2
then
    echo "usage: $0 <program-name> nproc> <args>"
    exit 1;
fi

pgm=$1
np=$2

shift
shift

i=0
while test $? -eq 0
do
    file=$(mktemp)
    echo "running to output $file : $i"
    ##echo mpirun -n $np $pgm "$@" 2>&1
    i=$(($i + 1))
    mpirun -n "$np" "$pgm" "$@" 2>&1 > "$file"
done
