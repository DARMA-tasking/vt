cmake_minimum_required(VERSION 3.8)
project(virtual_transport_mpi)

find_package(MPI REQUIRED)
if (MPI_FOUND)
    #        set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
    #        set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
    include_directories(MPI_INCLUDE_PATH)
else (MPI_FOUND)
    message(SEND_ERROR "This application cannot compile without MPI")
endif (MPI_FOUND)

set(CMAKE_CXX_STANDARD 14)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

set(
  PROJECT_SUBDIRS_LIST
  barrier collective context debug event messaging termination tree
  virtualization pool data registry parameterization
)

include_directories(${PROJECT_SOURCE_DIR})

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

# message("Backend: Headers LST: ${HEADER_FILES}")
# message("Backend: Source LST: ${SOURCE_FILES}")

set(
  PROJECT_EXAMPLES_LIST
  vt_mpi hello_world ring rdma_simple_get rdma_simple_get_direct rdma_simple_put
  rdma_simple_put_direct rdma_simple_channel rdma_channel_sync rdma_collective
  callback callback_new reinstate tagged_handler param
)

#message("HEADER_FILES=${HEADER_FILES}")

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

  target_link_libraries(
    ${EXAMPLE}
    ${VIRTUAL_TRANSPORT_LIBRARY}
  )

endforeach()
