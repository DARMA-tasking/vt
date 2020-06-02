################################################################################
#                              CMake for gtests                                #
################################################################################


set(PROJECT_TEST_UNIT_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(PROJECT_TEST_PERF_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/perf)
set(PROJECT_GTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/googletest/googletest/include)

set(
  PROJECT_PERF_TESTS
  ping_pong
  memory_checker
  comm_cost_curve
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
  runtime
  scheduler
  lb
  trace
  rdma
  utils
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
      #  STATUS "Building unit test >>>> dir=${SUB_DIR} test=${UNIT_TEST}, "
      #  "path=${UNIT_TEST_FILE}"
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
      target_include_directories(${UNIT_TEST} PRIVATE ${PROJECT_GTEST_INCLUDE_DIR})

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

#
# Performance Tests
#

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
