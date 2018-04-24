################################################################################
#                              CMake for gtests                                #
################################################################################

set(PROJECT_TEST_UNIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(PROJECT_TEST_PERF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/perf)

set(
  UNIT_TEST_SUBDIRS_LIST
  index
  active
  collectives
  pool
  sequencer
  mapping
  serialization
  tls
  atomic
)

option(CMAKE_NO_BUILD_TESTS "Enable VT tests to be built" OFF)

if (${VT_HAS_GTEST} AND NOT ${CMAKE_NO_BUILD_TESTS})
  set(UNIT_TEST_PROC_TEST_LIST 2 4 8)

  set(
    TEST_HEADER_FILES
    ${PROJECT_TEST_UNIT_DIR}/test_harness.h
  )

  set(
    TEST_SOURCE_FILES
    ${PROJECT_TEST_UNIT_DIR}/main.cc
  )

  include_directories(${PROJECT_TEST_UNIT_DIR})

  foreach(SUB_DIR ${UNIT_TEST_SUBDIRS_LIST})
    file(
      GLOB
      "${SUB_DIR}_UNIT_TEST_SOURCE_FILES"
      RELATIVE
      "${PROJECT_TEST_UNIT_DIR}"
      "${PROJECT_TEST_UNIT_DIR}/${SUB_DIR}/*.cc"
    )

    set(CUR_TEST_LIST "${SUB_DIR}_test_list")

    foreach (UNIT_TEST_FILE ${${SUB_DIR}_UNIT_TEST_SOURCE_FILES})
      # message(
      #   STATUS "Building unit test >>>> dir=${SUB_DIR} test=${UNIT_TEST}, "
      #   "path=${UNIT_TEST_FILE}"
      # )

      GET_FILENAME_COMPONENT(
        UNIT_TEST
        ${UNIT_TEST_FILE}
        NAME_WE
      )

      add_executable(
        ${UNIT_TEST}
        ${TEST_SOURCE_FILES}
        ${TEST_HEADER_FILES}
        ${PROJECT_TEST_UNIT_DIR}/${UNIT_TEST_FILE}
      )

      add_dependencies(unit_tests ${UNIT_TEST})

      link_target_with_vt(
        TARGET ${UNIT_TEST}
        DEFAULT_LINK_SET
        LINK_GTEST ON
      )

      foreach(PROC ${UNIT_TEST_PROC_TEST_LIST})
        gtest_add_tests(
          TARGET                 ${UNIT_TEST}
          WORKING_DIRECTORY      ${CMAKE_CURRENT_BINARY_DIR}
          TEST_SUFFIX            _proc_${PROC}
          TEST_LIST              ${CUR_TEST_LIST}
          EXECUTE_COMMAND        ${MPI_RUN_COMMAND} ${MPI_NUMPROC_FLAG} ${PROC}
        )
      endforeach()

      set_tests_properties(
        ${${CUR_TEST_LIST}}
          PROPERTIES TIMEOUT 60
          FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is;WARNING"
          PASS_REGULAR_EXPRESSION "PASSED"
        )
    endforeach()
  endforeach()

  # foreach (UNIT_TEST_FILE ${UNIT_TEST_SOURCE_FILES})
  # endforeach()
else()
  if (${CMAKE_NO_BUILD_TESTS})
    message(
      STATUS "VT: not building tests because CMAKE_NO_BUILD_TESTS is set"
    )
  else()
    message(STATUS "VT: not building tests because not VT_HAS_GTEST")
  endif()
endif()

foreach(PERF_TEST ${PROJECT_PERF_TESTS})
  add_executable(
    ${PERF_TEST}
    ${PROJECT_TEST_PERF_DIR}/${PERF_TEST}.cc
  )
  include_directories(${PERF_TEST} ${CMAKE_SOURCE_DIR}/src)
  include_directories(${PERF_TEST} ${CMAKE_SOURCE_DIR}/lib)
  include_directories(${PERF_TEST} ${CMAKE_BINARY_DIR})

  add_dependencies(perf_tests ${PERF_TEST})

  add_test_for_example_vt(
    ${PERF_TEST}
    ${CMAKE_CURRENT_BINARY_DIR}/${PERF_TEST}
  )

  link_target_with_vt(TARGET ${PERF_TEST} DEFAULT_LINK_SET)
endforeach()

