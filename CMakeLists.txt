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

set(
  PROJECT_SUBDIRS_LIST
  barrier collective context debug event messaging termination tree virtualization
)

include_directories(${PROJECT_SOURCE_DIR})

file(
  GLOB
  HEADER_FILES
  ${PROJECT_SOURCE_DIR}/*.h
)

foreach(SUB_DIR ${PROJECT_SUBDIRS_LIST})
  #message("Backend: Adding header of subdirectory >>>>> ${SUB_DIR}")

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

file(
  GLOB
  TOP_SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/*.cc
)

list(
  APPEND
  HEADER_FILES
  "${TOP_HEADER_FILES}"
)

list(
  APPEND
  SOURCE_FILES
  "${TOP_SOURCE_FILES}"
)

# message("Backend: Headers LST: ${HEADER_FILES}")
# message("Backend: Source LST: ${SOURCE_FILES}")

add_executable(
  vt_mpi
  ${SOURCE_FILES}
  ${HEADER_FILES}
)

target_link_libraries(
  vt_mpi
  PRIVATE
  ${MPI_C_LIBRARIES}
)

target_include_directories(
  vt_mpi
  PRIVATE
  ${MPI_C_INCLUDE_PATH}
)
