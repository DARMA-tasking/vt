# This file is intented to be run with cmake -P

# Derived from
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2009-2010.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

message(STATUS "Reading head file ${HEAD_FILE}")
message(STATUS "Using git executable at \"${GIT_EXECUTABLE}\"")

set(ROOT_DIR)
if (GIT_DIR)
    get_filename_component(ROOT_DIR ${GIT_DIR} DIRECTORY)
endif()

set(GIT_SHA1)

file(READ "${HEAD_FILE}" HEAD_CONTENTS LIMIT 1024)

string(STRIP "${HEAD_CONTENTS}" HEAD_CONTENTS)
if(HEAD_CONTENTS MATCHES "ref")
    # named branch
    string(REPLACE "ref: " "" GIT_REFSPEC "${HEAD_CONTENTS}")
endif()

message(STATUS "GIT_REFSPEC: \"${GIT_REFSPEC}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        rev-parse --verify HEAD
        WORKING_DIRECTORY
        "${ROOT_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_SHA1
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT res EQUAL 0)
    message(STATUS "could not get the git sha1")
endif()

message(STATUS "GIT_SHA1: \"${GIT_SHA1}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe
        ${GIT_SHA1}
        --tags --abbrev=0 --all
        WORKING_DIRECTORY
        "${ROOT_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_EXACT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT res EQUAL 0)
    message(STATUS "could not get the exact git tag")
endif()

message(STATUS "GIT_EXACT_TAG: \"${GIT_EXACT_TAG}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe
        ${GIT_SHA1}
        --abbrev=10 --always --tags --long --all
        WORKING_DIRECTORY
        "${ROOT_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_DESCRIPTION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT res EQUAL 0)
    message(STATUS "could not get the description")
endif()

message(STATUS "GIT_DESCRIPTION: \"${GIT_DESCRIPTION}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        diff-index --quiet HEAD --
        WORKING_DIRECTORY
        "${ROOT_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(res EQUAL 0)
    set(GIT_CLEAN_STATUS "CLEAN")
else()
    set(GIT_CLEAN_STATUS "DIRTY")
endif()

message(STATUS "Git Clean Status: \"${GIT_CLEAN_STATUS}\"")

message(STATUS "Configuring ${IN_FILE} to generate ${OUT_FILE}.")
configure_file(${IN_FILE} ${OUT_FILE} @ONLY)
