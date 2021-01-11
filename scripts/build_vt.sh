#!/usr/bin/env bash

if test $# -lt 1
then
    echo                                                \
        "usage: $0 "                                    \
       "<build-mode=debug|release|relwithdebinfo> "     \
       "<compiler=clang|gnu> "                          \
       "[ <has-serialization=0|1> ] "                   \
       "[ <build-all-tests=0|1> ] "                     \
       "[ <vt-install-directory> ] "                    \
       "[ <detector-path> ] "                           \
       "[ <checkpoint-path> ] "
    exit 29
fi

build_mode=$1

if test $# -gt 1
then
    compiler=$2
else
    compiler="clang"
fi

if test $# -gt 2
then
    has_serial=$3
else
    has_serial=1
fi

if test $# -gt 3
then
    has_all=$4
else
    has_all=1
fi

#echo "has_all=${has_all} has_serial=${has_serial} compiler=${compiler}"

if test ${has_serial} -gt 0
then
    serialization_dir=/Users/jliffla/codes/vt/checkpoint-install/
    if test $# -gt 6
    then
        serialization_dir=$7
    fi
fi

detector_dir=/Users/jliffla/codes/vt/detector-install

if test $# -gt 4
then
    vt_install_dir=$5
else
    vt_install_dir=../vt-install
fi

if test $# -gt 5
then
    detector_dir=$6
fi

SOURCE_BASE_DIR=../virtual-transport/
MAKE_VERBOSE=off

if test $compiler = "clang"
then
    CXX_COMPILER=clang++-mp-3.9
    CC_COMPILER=clang-mp-3.9
elif test $compiler = "gnu"
then
    CXX_COMPILER=mpicxx-mpich-devel-gcc6
    CC_COMPILER=mpicc-mpich-devel-gcc6
else
    (>&2 echo "Please specify valid compiler option: $compiler")
    exit 10
fi

if test ${has_all} -gt 0
then
    build_all=""
else
    #echo "Setting no build all for ${has_all}"
    build_all="-DVT_BUILD_TESTS=0 -DVT_BUILD_EXAMPLES=0"
fi

(>&2 echo "=== Building vt ===")
(>&2 echo -e "\tBuild mode:$build_mode")
(>&2 echo -e "\tCompiler suite=$compiler, cxx=$CXX_COMPILER, cc=$CC_COMPILER")
(>&2 echo -e "\tAll tests/examples=${has_all}")
(>&2 echo -e "\tVT installation directory=${vt_install_dir}")
(>&2 echo -e "\tCheckpoint=${has_serial}, path=$serialization_dir")
(>&2 echo -e "\tDetector path=${detector_dir}")

cmake ${SOURCE_BASE_DIR}                                                    \
      -DCMAKE_INSTALL_PREFIX="${vt_install_dir}"                            \
      -DCMAKE_BUILD_TYPE="${build_mode}"                                    \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL="${MAKE_VERBOSE}"                       \
      -DCMAKE_CXX_COMPILER="${CXX_COMPILER}"                                \
      -DCMAKE_C_COMPILER="${CC_COMPILER}"                                   \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=true                                  \
      ${build_all}                                                          \
      -Dcheckpoint_DIR="${serialization_dir}"                               \
      -Ddetector_DIR="${detector_dir}"
