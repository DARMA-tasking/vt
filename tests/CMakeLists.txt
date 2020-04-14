################################################################################
#                              CMake for gtests                                #
################################################################################

set(PROJECT_TEST_UNIT_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(PROJECT_TEST_PERF_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/perf)

set(
  PROJECT_PERF_TESTS
  ping_pong
  memory_checker
)

set(
  UNIT_TEST_SUBDIRS_LIST
  index
  active
  collection
  collectives
  epoch
  pool
  location
  sequencer
  termination
  mapping
  group
  serialization
  pipe
  tls
  atomic
  memory
  objgroup
  scheduler
  lb
  trace
)

option(VT_NO_BUILD_TESTS "Disable building VT tests" OFF)

if (${VT_HAS_GTEST} AND NOT ${VT_NO_BUILD_TESTS})
  set(
    TEST_HEADER_FILES
    ${PROJECT_TEST_UNIT_DIR}/test_harness.h
  )

  set(
    TEST_SOURCE_FILES
    ${PROJECT_TEST_UNIT_DIR}/main.cc
  )

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

      target_include_directories(${UNIT_TEST} PRIVATE ${PROJECT_TEST_UNIT_DIR})

      link_target_with_vt(
        TARGET ${UNIT_TEST}
        DEFAULT_LINK_SET
        LINK_GTEST ON
      )

      foreach(PROC ${PROC_TEST_LIST})
        gtest_add_tests(
          TARGET                 ${UNIT_TEST}
          WORKING_DIRECTORY      ${CMAKE_CURRENT_BINARY_DIR}
          TEST_SUFFIX            _proc_${PROC}
          TEST_PREFIX            vt:
          TEST_LIST              ${CUR_TEST_LIST}
          EXECUTE_COMMAND        ${MPI_RUN_COMMAND} ${MPI_EXTRA_FLAGS} ${MPI_NUMPROC_FLAG} ${PROC}
        )

        set_tests_properties(
          ${${CUR_TEST_LIST}}
          PROPERTIES TIMEOUT 60
          FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is;Segmentation fault"
          PASS_REGULAR_EXPRESSION "PASSED"
        )
    endforeach()
      endforeach()

  endforeach()
else()
  if (${VT_NO_BUILD_TESTS})
    message(
      STATUS
      "VT: not building unit tests because VT_NO_BUILD_TESTS is set"
    )
  else()
    message(STATUS "VT: not building unit tests because not VT_HAS_GTEST")
  endif()
endif()

if (${VT_NO_BUILD_TESTS})
  message(
    STATUS
    "VT: not building performance tests because VT_NO_BUILD_TESTS is set"
  )
else()
  foreach(PERF_TEST ${PROJECT_PERF_TESTS})
    add_executable(${PERF_TEST} ${PROJECT_TEST_PERF_DIR}/${PERF_TEST}.cc)
    add_dependencies(perf_tests ${PERF_TEST})

    link_target_with_vt(
      TARGET ${PERF_TEST}
      DEFAULT_LINK_SET
      LINK_GTEST ON
    )

    add_test_for_example_vt(
      ${PERF_TEST}
      ${PROJECT_TEST_PERF_DIR}/${PERF_TEST}.cc
      perf_tests
    )
  endforeach()
endif()

