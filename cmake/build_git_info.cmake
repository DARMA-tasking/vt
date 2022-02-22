function(get_git_dir _git_dir)
    set(GIT_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GIT_DIR "${GIT_PARENT_DIR}/.git")
    while(NOT EXISTS "${GIT_DIR}")	# .git dir not found, search parent directories
        set(GIT_PREVIOUS_PARENT "${GIT_PARENT_DIR}")
        get_filename_component(GIT_PARENT_DIR ${GIT_PARENT_DIR} PATH)
        if(GIT_PARENT_DIR STREQUAL GIT_PREVIOUS_PARENT)
            # We have reached the root directory, we are not in git
            set(${_refspecvar} "GITDIR-NOTFOUND" PARENT_SCOPE)
            set(${_hashvar} "GITDIR-NOTFOUND" PARENT_SCOPE)
            return()
        endif()
        set(GIT_DIR "${GIT_PARENT_DIR}/.git")
    endwhile()
    # check if this is a submodule
    if(NOT IS_DIRECTORY ${GIT_DIR})
        file(READ ${GIT_DIR} submodule)
        string(REGEX REPLACE "gitdir: (.*)\n$" "\\1" GIT_DIR_RELATIVE ${submodule})
        get_filename_component(SUBMODULE_DIR ${GIT_DIR} PATH)
        get_filename_component(GIT_DIR ${SUBMODULE_DIR}/${GIT_DIR_RELATIVE} ABSOLUTE)
    endif()

    set(${_git_dir} ${GIT_DIR} PARENT_SCOPE)
endfunction()

get_git_dir(GIT_DIR)

if(NOT EXISTS "${GIT_DIR}/HEAD")
    message(FATAL_ERROR "no such file: \"${GIT_DIR}/HEAD\"")
endif()
set(HEAD_FILE "${GIT_DIR}/HEAD")

message(STATUS "Git HEAD file: \"${HEAD_FILE}\"")

find_package(Git REQUIRED)
#execute_process(COMMAND ${CMAKE_COMMAND} -DIN_FILE=${PROJECT_BASE_DIR}/vt_git_revision.cc.in -DOUT_FILE=${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DGIT_DIR=${GIT_DIR} -DHEAD_FILE=${HEAD_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/run-git.cmake)

set(VT_GIT_CONFIG_FILE "${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc")
add_custom_command(
        OUTPUT ${VT_GIT_CONFIG_FILE}
        COMMAND ${CMAKE_COMMAND} -DIN_FILE=${PROJECT_BASE_DIR}/vt_git_revision.cc.in -DOUT_FILE=${VT_GIT_CONFIG_FILE} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DGIT_DIR=${GIT_DIR} -DHEAD_FILE=${HEAD_FILE} -P ${CMAKE_CURRENT_LIST_DIR}/run-git.cmake
        DEPENDS ${GIT_DIR}
        )

target_sources(${VIRTUAL_TRANSPORT_LIBRARY} PRIVATE ${VT_GIT_CONFIG_FILE})