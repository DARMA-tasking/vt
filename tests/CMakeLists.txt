################################################################################
#                              CMake for gtests                                #
################################################################################

set(PROJECT_TEST_UNIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unit)
set(PROJECT_TEST_PERF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/perf)

set(
  UNIT_TEST_SUBDIRS_LIST
  index active collectives pool sequencer mapping serialization tls atomic
)

foreach(SUB_DIR ${UNIT_TEST_SUBDIRS_LIST})
  file(
    GLOB
    "${SUB_DIR}_UNIT_TEST_SOURCE_FILES"
    RELATIVE
    "${PROJECT_TEST_UNIT_DIR}"
    "${PROJECT_TEST_UNIT_DIR}/${SUB_DIR}/*.cc"
  )

  list(
    APPEND
    UNIT_TEST_SOURCE_FILES
    "${${SUB_DIR}_UNIT_TEST_SOURCE_FILES}"
  )
endforeach()

if (NOT GTEST_DIR)
  message("Unit test are disabled because GoogleTest is not setup via -DGTEST_DIR=")
else()
  set(
    TEST_HEADER_FILES
    ${PROJECT_TEST_UNIT_DIR}/test_harness.h
  )

  set(
    TEST_SOURCE_FILES
    ${PROJECT_TEST_UNIT_DIR}/main.cc
  )

  include_directories(${GTEST_DIR}/include)
  include_directories(${PROJECT_TEST_UNIT_DIR})

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")

  set(UNIT_TEST_PROC_TEST_LIST 2 4 8)

  foreach (UNIT_TEST_FILE ${UNIT_TEST_SOURCE_FILES})
    GET_FILENAME_COMPONENT(
      UNIT_TEST
      ${UNIT_TEST_FILE}
      NAME_WE
    )

    #message("Build unit test >>>> test=${UNIT_TEST}, path=${UNIT_TEST_FILE}")

    if(CMAKE_NO_BUILD_TESTS)
      message("Skipping test: ${UNIT_TEST}_${PROC}")
    else()
      add_executable(
        ${UNIT_TEST}
        ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES}
        ${PROJECT_TEST_UNIT_DIR}/${UNIT_TEST_FILE}
        )

      add_dependencies(unit_tests ${UNIT_TEST})

      target_link_libraries(
        ${UNIT_TEST}
        ${VIRTUAL_TRANSPORT_LIBRARY}
        ${ZLIB_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${FCONTEXT_LIBRARY}
        ${SERIALIZATION_LIBRARY}
        gtest gmock
      )

      foreach(PROC ${UNIT_TEST_PROC_TEST_LIST})
        add_test(
          ${UNIT_TEST}_${PROC}
          ${MPI_RUN_COMMAND} ${MPI_NUMPROC_FLAG} ${PROC}
          ${CMAKE_CURRENT_BINARY_DIR}/${UNIT_TEST}
        )
        set_tests_properties(
          ${UNIT_TEST}_${PROC}
          PROPERTIES TIMEOUT 60
          FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is;WARNING"
          PASS_REGULAR_EXPRESSION "PASSED"
        )
      endforeach()
    endif()
  endforeach()
endif()

foreach(PERF_TEST ${PROJECT_PERF_TESTS})
  add_executable(
    ${PERF_TEST}
    ${PROJECT_TEST_PERF_DIR}/${PERF_TEST}.cc
  )
  include_directories(${PERF_TEST} ${CMAKE_SOURCE_DIR}/src)
  include_directories(${PERF_TEST} ${CMAKE_SOURCE_DIR}/lib)
  include_directories(${PERF_TEST} ${CMAKE_BINARY_DIR})
  include_directories(${PERF_TEST} ${DARMA_DETECTOR_PATH}/include)
  include_directories(${PERF_TEST} ${DARMA_MELD_PATH}/include)

  add_dependencies(perf_tests ${PERF_TEST})

  # add_test_for_example(
  #   ${PERF_TEST}
  #   ${CMAKE_CURRENT_BINARY_DIR}/${PERF_TEST}
  # )

  target_link_libraries(
    ${PERF_TEST}
    ${VIRTUAL_TRANSPORT_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${MPI_C_LIBRARIES}
    ${FCONTEXT_LIBRARY}
    ${SERIALIZATION_LIBRARY}
  )
endforeach()

