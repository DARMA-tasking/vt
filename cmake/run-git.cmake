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

set(GIT_SHA1)

file(READ "${HEAD_FILE}" HEAD_CONTENTS LIMIT 1024)

string(STRIP "${HEAD_CONTENTS}" HEAD_CONTENTS)
if(HEAD_CONTENTS MATCHES "ref")
    # named branch
    string(REPLACE "ref: " "" GIT_REFSPEC "${HEAD_CONTENTS}")
    if(EXISTS "${GIT_DIR}/${GIT_REFSPEC}")
        file(READ "${GIT_DIR}/${GIT_REFSPEC}" GIT_SHA1 LIMIT 1024)
        string(STRIP "${GIT_SHA1}" GIT_SHA1)
    else()
        file(READ "${GIT_DIR}/packed-refs" PACKED_REFS)
        if(${PACKED_REFS} MATCHES "([0-9a-z]*) ${GIT_REFSPEC}")
            set(GIT_SHA1 "${CMAKE_MATCH_1}")
        endif()
    endif()
else()
    # detached HEAD
    string(STRIP "${HEAD_CONTENTS}" GIT_SHA1)
endif()

message(STATUS "GIT_REFSPEC: \"${GIT_REFSPEC}\"")
message(STATUS "GIT_SHA1: \"${GIT_SHA1}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe
        ${GIT_SHA1}
        --tags --abbrev=0 --all
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_EXACT_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "could not get the exact git tag")
endif()

message(STATUS "GIT_EXACT_TAG: \"${GIT_EXACT_TAG}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe
        ${GIT_SHA1}
        --abbrev=10 --always --tags --long --all
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        GIT_DESCRIPTION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "could not get the description")
endif()

message(STATUS "GIT_DESCRIPTION: \"${GIT_DESCRIPTION}\"")

execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        diff-index --quiet HEAD --
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if(res EQUAL 0)
    set(GIT_CLEAN_STATUS "CLEAN")
else()
    set(GIT_CLEAN_STATUS "DIRTY")
endif()

message(STATUS "Git Clean Status: \"${GIT_CLEAN_STATUS}\"")

message(STATUS "Configuring ${IN_FILE} to generate ${OUT_FILE}.")
configure_file(${IN_FILE} ${OUT_FILE} @ONLY)