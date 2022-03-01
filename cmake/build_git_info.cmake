find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
        WORKING_DIRECTORY
        "${PROJECT_BASE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        REL_GIT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT res EQUAL 0)
    message(FATAL_ERROR "git invocation failed")
endif()

get_filename_component(GIT_DIR ${REL_GIT_DIR} ABSOLUTE BASE_DIR ${PROJECT_BASE_DIR})
message(STATUS "Git DIR: ${GIT_DIR}")
if (NOT GIT_DIR)
    message(FATAL_ERROR "not a git directory")
endif()

if(NOT EXISTS "${GIT_DIR}/HEAD")
    message(FATAL_ERROR "no such file: \"${GIT_DIR}/HEAD\"")
endif()
set(HEAD_FILE "${GIT_DIR}/HEAD")

message(STATUS "Git HEAD file: \"${HEAD_FILE}\"")

set(VT_GIT_CONFIG_FILE "${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc")
add_custom_command(
        OUTPUT ${VT_GIT_CONFIG_FILE}
        COMMAND ${CMAKE_COMMAND} -DIN_FILE=${PROJECT_BASE_DIR}/vt_git_revision.cc.in -DOUT_FILE=${VT_GIT_CONFIG_FILE} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DGIT_DIR=${GIT_DIR} -DHEAD_FILE=${HEAD_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/run-git.cmake
        DEPENDS ${GIT_DIR}
        )

target_sources(${VIRTUAL_TRANSPORT_LIBRARY} PRIVATE ${VT_GIT_CONFIG_FILE})
