#!/bin/bash

dir=$1

echo "dir=$dir"

#find $dir -iname "*.h" -exec  {} \;
#cmd='sed s/include\ \"/include\ \"vt\\//'

find $dir -iname "*.cc" -exec sed -i '' s/include\ \"trans/include\ \"vt\\/trans/ {} \;

