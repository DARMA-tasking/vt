################################################################################
#                              CMake for gtests                                #
################################################################################

option(vt_build_extended_tests "Build the extended testing for VT" ON)

if(vt_build_extended_tests)
  message(STATUS "Building VT with extended testing")
else()
  message(STATUS "Building VT without extended testing")
endif()

set(PROJECT_TEST_UNIT_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(PROJECT_TEST_PERF_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/perf)
set(PROJECT_GTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/googletest/googletest/include)

function(subdirlist result curdir)
  file(
    GLOB children
    LIST_DIRECTORIES true
    RELATIVE ${curdir} ${curdir}/*
  )
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist} PARENT_SCOPE)
endfunction()

subdirlist(
  UNIT_TEST_SUBDIRS_LIST
  ${PROJECT_TEST_UNIT_DIR}
)


#
# Setup/add googletest CMake configuration
#

# Add dependency googletest CMake - gtest is exported via 'cxx_library'.
# Override googletest options.
set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)

add_subdirectory(extern/googletest)

# Hide various options from UI-based property editors
mark_as_advanced(
  BUILD_GTEST INSTALL_GTEST BUILD_SHARED_LIBS
  gtest_build_samples gtest_build_tests
  gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
  #BUILD_GMOCK
  #gmock_build_tests
)

# Group targets, in IDEs supporting such
set_target_properties(gtest PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
#set_target_properties(gmock PROPERTIES FOLDER extern)
#set_target_properties(gmock_main PROPERTIES FOLDER extern)

include(GoogleTest)

function(add_unit_test unit_test_name unit_test_files uses_mpi)
  add_executable(
    ${unit_test_name}
    ${TEST_SOURCE_FILES}
    ${TEST_HEADER_FILES}
    ${${unit_test_files}}
  )

  add_dependencies(unit_tests ${unit_test_name})

  target_include_directories(${unit_test_name} PRIVATE ${PROJECT_TEST_UNIT_DIR})
  target_include_directories(${unit_test_name} PRIVATE ${PROJECT_GTEST_INCLUDE_DIR})

  link_target_with_vt(
    TARGET ${unit_test_name}
    DEFAULT_LINK_SET
    LINK_GTEST ON
  )

  if (vt_unity_build_enabled)
    set_target_properties(${unit_test_name} PROPERTIES UNITY_BUILD ON)
  endif()

  if(uses_mpi)
    foreach(PROC ${PROC_TEST_LIST})
      gtest_add_tests(
        TARGET                 ${unit_test_name}
        WORKING_DIRECTORY      ${CMAKE_CURRENT_BINARY_DIR}
        TEST_SUFFIX            _proc_${PROC}
        TEST_PREFIX            vt:
        TEST_LIST              ${CUR_TEST_LIST}
        EXECUTE_COMMAND        ${MPI_RUN_COMMAND} ${MPI_EXTRA_FLAGS_LIST} ${MPI_NUMPROC_FLAG} ${PROC}
      )

      set_tests_properties(
        ${${CUR_TEST_LIST}}
        PROPERTIES TIMEOUT 60
        FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is;Segmentation fault"
        PASS_REGULAR_EXPRESSION "PASSED"
        SKIP_REGULAR_EXPRESSION "SKIPPED"
      )
    endforeach()
  else()
    gtest_add_tests(
      TARGET                 ${unit_test_name}
      WORKING_DIRECTORY      ${CMAKE_CURRENT_BINARY_DIR}
      TEST_SUFFIX            _no_mpi
      TEST_PREFIX            vt:
      TEST_LIST              ${CUR_TEST_LIST}
    )
  endif()

endfunction()

#
# Unit Tests
#

set(
  TEST_HEADER_FILES
  ${PROJECT_TEST_UNIT_DIR}/test_harness.h
)

set(
  TEST_SOURCE_FILES
  ${PROJECT_TEST_UNIT_DIR}/main.cc
)

# List of test files that should be excluded
set(
  EXCLUDED_TEST_FILES
  ${PROJECT_TEST_UNIT_DIR}/sequencer/test_sequencer_extensive.extended.cc
)

foreach(SUB_DIR ${UNIT_TEST_SUBDIRS_LIST})
  file(
    GLOB
    "${SUB_DIR}_UNIT_TEST_SOURCE_FILES"
    RELATIVE
    ""
    "${PROJECT_TEST_UNIT_DIR}/${SUB_DIR}/*.cc"
  )

  set(CUR_TEST_LIST "${SUB_DIR}_test_list")

  set(UNIT_LIST_EXTENDED "")
  set(UNIT_LIST_BASIC "")
  set(UNIT_LIST_NOMPI "")

  foreach (unit_test_file ${${SUB_DIR}_UNIT_TEST_SOURCE_FILES})
    #message(STATUS "Considering ${unit_test_file}")

    if (${unit_test_file} IN_LIST EXCLUDED_TEST_FILES)
      message(STATUS "Test file ${unit_test_file} is present on EXCLUDED_TEST_FILES list. Skipping!")
      continue()
    endif()

    GET_FILENAME_COMPONENT(
      UNIT_TEST
      ${unit_test_file}
      NAME_WE
    )

    GET_FILENAME_COMPONENT(
      UNIT_TEST_FULL_EXTENSION
      ${unit_test_file}
      EXT
    )

    # Extended tests are designated with an particular extension: *.extended.cc
    if(UNIT_TEST_FULL_EXTENSION MATCHES "[.]extended[.]")
      list(APPEND UNIT_LIST_EXTENDED ${unit_test_file})
    else()
      if(UNIT_TEST_FULL_EXTENSION MATCHES "[.]nompi[.]")
        list(APPEND UNIT_LIST_NOMPI ${unit_test_file})
      else()
        list(APPEND UNIT_LIST_BASIC ${unit_test_file})
      endif()
    endif()
  endforeach()

  add_unit_test("${SUB_DIR}_basic" UNIT_LIST_BASIC ON)
  add_unit_test("${SUB_DIR}_nompi" UNIT_LIST_NOMPI OFF)

  if (vt_build_extended_tests)
    add_unit_test("${SUB_DIR}_extended" UNIT_LIST_EXTENDED ON)
  endif()
endforeach()

#
# Performance Tests
#

add_library(perf_timer STATIC
  ${PROJECT_TEST_PERF_DIR}/common/timers.h
  ${PROJECT_TEST_PERF_DIR}/common/timers.cc
)

file(GLOB PROJECT_PERF_TESTS ${PROJECT_TEST_PERF_DIR}/*.cc)

foreach(PERF_TEST ${PROJECT_PERF_TESTS})
  GET_FILENAME_COMPONENT(
    TEST_NAME
    ${PERF_TEST}
    NAME_WLE
  )

  add_executable(${TEST_NAME} ${PERF_TEST}
    ${PROJECT_TEST_PERF_DIR}/common/test_harness.h
    ${PROJECT_TEST_PERF_DIR}/common/test_harness.cc
  )

  link_target_with_vt(
    TARGET ${TEST_NAME}
    DEFAULT_LINK_SET
  )

  target_link_libraries(${TEST_NAME} PUBLIC perf_timer)

endforeach()
