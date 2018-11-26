#!/bin/bash

dir=$1

license=$2

x=$(find $dir -iname "*.h")
for i in $x; do
   cat $license > "$license"temp
   f=$(basename $i)
   sed -i '' 's/\<file-name.h\>/'$f'/g' "license"temp
   cat "$license"temp $i > "$i"temp
   cat "$i"temp > $i
   rm "$license"temp
   rm "$i"temp
done