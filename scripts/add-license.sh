#!/bin/bash

if test $# -lt 2
then
    echo "usage: $0 <root-dir> <license-template> [pattern]"
    exit 1
fi

dir=$1
license=$2

if test $# -gt 2
then
    pattern=$3
else
    pattern="*.h"
fi

echo "Running on root=$dir, license=$license, pattern=$pattern"

allfiles=$(find $dir -iname "$pattern")

for file in $allfiles
do
   filename=$(basename $file)
   filedir=$(dirname $file)
   licensetmp=$(mktemp)
   filetmp=$(mktemp)
   echo "Running on file=$file, dir=$filedir, license=$license, tmp=$tmp"
   sed 's/\<file-name.h\>/'$filename'/g' $license > $licensetmp
   cat $licensetmp $file > $filetmp
   mv $filetmp $file
done
