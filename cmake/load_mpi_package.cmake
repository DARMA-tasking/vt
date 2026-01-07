
#
#  Load and discover MPI settings (required)
#

# Add a user-configurable option to control whether find_package(MPI) is called
option(vt_find_mpi "Enable find_package(MPI) (default: ON)" ON)

if(vt_find_mpi)
  message(STATUS "Using find_package(MPI)")
  find_package(MPI REQUIRED)
  if(MPI_FOUND)
    # include_directories(${MPI_INCLUDE_PATH})  # Uncomment if needed
  else()
    message(FATAL_ERROR "Failure to locate MPI: MPI is required for VT to build")
  endif()
else()
  message(STATUS "Skipping find_package(MPI). Ensure MPI compiler wrappers are used.")

  # Provide reasonable defaults for MPI variables
  if(NOT DEFINED MPIEXEC_EXECUTABLE)
    set(MPIEXEC_EXECUTABLE "mpirun")  # Default to mpirun
    message(WARNING "MPIEXEC_EXECUTABLE not set. Defaulting to 'mpirun'.")
  endif()

  if(NOT DEFINED MPIEXEC_NUMPROC_FLAG)
    set(MPIEXEC_NUMPROC_FLAG "-n")  # Default to -n for number of processes
    message(WARNING "MPIEXEC_NUMPROC_FLAG not set. Defaulting to '-n'.")
  endif()

  if(NOT DEFINED MPIEXEC_MAX_NUMPROCS)
    # Default to a safe value or detect system cores
    set(MPIEXEC_MAX_NUMPROCS 2)
    message(WARNING "MPIEXEC_MAX_NUMPROCS not set. Defaulting to '2'.")
  endif()
endif()

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

