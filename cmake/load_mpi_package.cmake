
#
#  Load and discover MPI settings (required)
#

find_package(MPI REQUIRED)
if(MPI_FOUND)
  #include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Failure to locate MPI: MPI is required for VT to build")
endif(MPI_FOUND)


# Set default command for invoking MPI (mpirun) and flag for MPI nprocs
set(MPI_RUN_COMMAND  "${MPIEXEC_EXECUTABLE}")
set(MPI_PRE_FLAGS    "${MPIEXEC_PREFLAGS}")
set(MPI_EPI_FLAGS    "${MPIEXEC_POSTFLAGS}")
set(MPI_NUMPROC_FLAG "${MPIEXEC_NUMPROC_FLAG}")

set(cmake_detected_max_num_nodes ${MPIEXEC_MAX_NUMPROCS})

set(MPI_MAX_NUMPROC "${MPIEXEC_MAX_NUMPROCS}")

if(${vt_tests_num_nodes})
  set(MPI_MAX_NUMPROC "${vt_tests_num_nodes}")
endif()

if(${MPI_MAX_NUMPROC} GREATER ${MPIEXEC_MAX_NUMPROCS})
  message(STATUS "Oversubscribing number of nodes to ${MPI_MAX_NUMPROC} with detected ${MPIEXEC_MAX_NUMPROCS}")
endif()

message(STATUS "MPI max nproc: ${MPI_MAX_NUMPROC}")

include(cmake/test_vt.cmake)

set(PROC_TEST_LIST "")
build_mpi_proc_test_list(
  MAX_PROC       ${MPI_MAX_NUMPROC}
  VARIABLE_OUT   PROC_TEST_LIST
)

message(STATUS "MPI proc test list: ${PROC_TEST_LIST}")
message(STATUS "MPI exec: ${MPIEXEC_EXECUTABLE}")

