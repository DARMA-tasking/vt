#!/bin/bash

cmake ../detector \
      -DCMAKE_INSTALL_PREFIX=`pwd` \
      -DCMAKE_CXX_COMPILER=clang++-mp-3.9 \
      -DCMAKE_C_COMPILER=clang-mp-3.9 \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=true
