cmake_minimum_required(VERSION 3.8)
project(virtual_transport_mpi)

find_package(MPI REQUIRED)

# set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
# set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

include_directories(${MPI_C_INCLUDE_PATH})

set(CMAKE_CXX_STANDARD 14)

# OpenMP support
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
include(${CMAKE_MODULE_PATH}/FindOpenMP.cmake)

if (OpenMP_FOUND)
  message("OpenMP has been found: ")
  message("\t Linker flags: ${OpenMP_EXE_LINKER_FLAGS}")
  message("\t C_FLAGS=${OpenMP_C_FLAGS}")
  message("\t CXX_FLAGS=${OpenMP_CXX_FLAGS}")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
  message("OpenMP not found: must use std::thread for workers.")
endif (OpenMP_FOUND)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(PROJECT_TEST_UNIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit)
set(PROJECT_TEST_PERF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/perf)

include (CTest)
enable_testing()

find_package(ZLIB REQUIRED)

if (ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message("zlib is required for tracing")
endif (ZLIB_FOUND)

set(
  PROJECT_SUBDIRS_LIST
  barrier collective configs/debug configs/features configs/types context
  event messaging pool data registry parameterization handler sequence
  termination trace tree scheduler utils/demangle utils/bits demangle
  location index mapping utils/container utils/detector serialization
)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_LIB_DIR})

add_subdirectory(${PROJECT_LIB_DIR}/context)

file(
  GLOB
  HEADER_FILES
  ${PROJECT_SOURCE_DIR}/*.h
)

foreach(SUB_DIR ${PROJECT_SUBDIRS_LIST})
  # message(
  #   "virtual-transport: adding header of subdirectory >>>>> ${SUB_DIR}\n"
  #   "\tsource=${CMAKE_CURRENT_SOURCE_DIR}\n"
  #   "\theader=${CMAKE_CURRENT_SOURCE_DIR}/src/${SUB_DIR}/*.h\n"
  #   "\tsource=${CMAKE_CURRENT_SOURCE_DIR}/src/${SUB_DIR}/*.cc"
  # )

  file(
    GLOB
    "${SUB_DIR}_HEADER_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${SUB_DIR}/*.h"
  )

  file(
    GLOB
    "${SUB_DIR}_SOURCE_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${SUB_DIR}/*.cc"
  )

  include_directories(
    ${PROJECT_SOURCE_DIR}/${SUB_DIR}
  )

  list(
    APPEND
    HEADER_FILES
    "${${SUB_DIR}_HEADER_FILES}"
  )

  list(
    APPEND
    SOURCE_FILES
    "${${SUB_DIR}_SOURCE_FILES}"
  )
endforeach()

file(
  GLOB
  TOP_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/*.h
)

list(
  APPEND
  HEADER_FILES
  "${TOP_HEADER_FILES}"
)

list(
  APPEND
  SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/runtime.cc
)

set(
  PROJECT_EXAMPLES_LIST
  hello_world ring rdma_simple_get rdma_simple_get_direct rdma_simple_put
  rdma_simple_put_direct rdma_simple_channel rdma_channel_sync rdma_collective
  callback callback_new reinstate tagged_handler param hello_world_functor
  test_seq trace_ring location_test location_test_msg broadcast_test index_test
  hello_world_vrtContext hello_world_vc jacobi1d_recur_vc jacobi1d_node
)

set(
  PROJECT_PERF_TESTS
  ping_pong
)

set(MPI_RUN_COMMAND "mpirun-mpich-clang")
set(MPI_NUMPROC_FLAG "-n")

set(PROC_TEST_LIST 2 4 8)

macro(add_test_for_example test_name test_exec)
  foreach(PROC ${PROC_TEST_LIST})
    add_test(
      ${test_name}_${PROC}
      ${MPI_RUN_COMMAND} ${MPI_NUMPROC_FLAG} ${PROC}
      ${test_exec} ${ARGN}
    )

    set_tests_properties(
      ${test_name}_${PROC}
      PROPERTIES TIMEOUT 300 FAIL_REGULAR_EXPRESSION "FAILED;WARNING"
    )
  endforeach()
endmacro()

set(VIRTUAL_TRANSPORT_LIBRARY virtual-transport)

add_library(
  ${VIRTUAL_TRANSPORT_LIBRARY}
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES}
)

foreach(EXAMPLE ${PROJECT_EXAMPLES_LIST})
  # message("Example: building example >>>>> ${EXAMPLE}")

  add_executable(
    ${EXAMPLE}
    ${PROJECT_EXAMPLE_DIR}/${EXAMPLE}.cc
  )

  add_test_for_example(
    ${EXAMPLE}
    ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}
  )

  target_link_libraries(
    ${EXAMPLE}
    ${VIRTUAL_TRANSPORT_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${MPI_C_LIBRARIES}
  )
endforeach()

foreach(PERF_TEST ${PROJECT_PERF_TESTS})
  add_executable(
    ${PERF_TEST}
    ${PROJECT_TEST_PERF_DIR}/${PERF_TEST}.cc
  )

  # add_test_for_example(
  #   ${PERF_TEST}
  #   ${CMAKE_CURRENT_BINARY_DIR}/${PERF_TEST}
  # )

  target_link_libraries(
    ${PERF_TEST}
    ${VIRTUAL_TRANSPORT_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${MPI_C_LIBRARIES}
  )
endforeach()

################################################################################
#                              CMake for gtests                                #
################################################################################

set(
  UNIT_TEST_SUBDIRS_LIST
  index active pool sequencer
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

#message("unit test source files: ${UNIT_TEST_SOURCE_FILES}")

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

    add_executable(
      ${UNIT_TEST}
      ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES}
      ${PROJECT_TEST_UNIT_DIR}/${UNIT_TEST_FILE}
    )

    target_link_libraries(
      ${UNIT_TEST}
      ${VIRTUAL_TRANSPORT_LIBRARY}
      ${ZLIB_LIBRARIES}
      ${MPI_C_LIBRARIES}
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

  endforeach()
endif()

