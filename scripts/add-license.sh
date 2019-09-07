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

licencePrefix1="/*"
licencePrefix2="//"
licencePrefix3="*/"


str2="Learn Bash"

for file in $allfiles
do
   filename=$(basename $file)
   filedir=$(dirname $file)
   licensetmp=$(mktemp)
   filetmp=$(mktemp)
   echo "Running on file=$file, dir=$filedir, license=$license, tmp=$filetmp"
   endCondition=0
   while [ "$endCondition" != "1" ]; do
     read -r firstline<$file
     id=${firstline:0:2}

     if [ "$id" == "$licencePrefix1" -o "$id" == "$licencePrefix2" -o "$id" == "$licencePrefix3" ]
     then
        sed -i.bak  '1d' "$file"
     else
        echo "NoLicense found"
        endCondition="1"
     fi
     if [ "$id" == "$licencePrefix3" ]
     then
        endCondition="1"
     fi
     #code for passing id to other script file as parameter
   done< "$file"
   rm $file.bak
   sed 's/\<file-name\>/'$filename'/g' $license > $licensetmp
   cat $licensetmp $file > $filetmp
   mv $filetmp $file
done
