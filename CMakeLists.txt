cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vt)

set(VIRTUAL_TRANSPORT_LIBRARY vt)
set(FCONTEXT_LIBRARY fcontext)

# Set the local module path so custom cmake scripts can be located automatically
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules/")

include(SetCXXCompilerFlags)
set_darma_compiler_flags()

include(local_package.cmake)
include(link_vt.cmake)
include(threading_config.cmake)
include(test_vt.cmake)

# require directories for these packages
require_pkg_directory(darma_meld     "DARMA meld")
require_pkg_directory(darma_detector "DARMA detector")
# find these required packages locally
find_package_local(darma_meld     "${darma_meld_DIR}/cmake")
find_package_local(darma_detector "${darma_detector_DIR}/cmake")
# optional directory for this package
optional_pkg_directory(darma_checkpoint "Serialization/Checkpoint")
# find the optional packages locally if identified
if (${darma_checkpoint_DIR_FOUND})
  find_package_local(darma_checkpoint "${darma_checkpoint_DIR}/cmake")
  if (${darma_checkpoint_FOUND})
    set(VT_HAS_SERIALIZATION_LIBRARY 1)
    set(CHECKPOINT_LIBRARY darma_checkpoint)
  else()
    message(FATAL_ERROR "Serialization/checkpoint library not found")
  endif()
endif()

find_package(MPI REQUIRED)
if(MPI_FOUND)
  #include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Failure to locate MPI: MPI is required for VT to build")
endif(MPI_FOUND)

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message("zlib is required for tracing")
endif(ZLIB_FOUND)

find_package(fmt PATHS ${fmt_DIR} ${fmt_DIR}/lib/cmake)
if(NOT fmt_FOUND)
  message(
    FATAL_ERROR
    "VT requires the fmt library. Please specify valid installation with "
    "-Dfmt_DIR= or install by downloading from https://github.com/fmtlib/fmt"
  )
endif()

# Google test cmake option and package setup
option(
  enable_gtest "Enable google test framework: -Dgtest_DIR= to specify path" ON
)

if (${enable_gtest})
  if (${gtest_DIR})
    set(GTEST_ROOT "${gtest_DIR}" CACHE PATH "Path to googletest")
  endif()

  # Instead of explicitly including (include(FindGTest)), call find_package
  # which automatically searches the module include path. This is the preferred
  # mechanism for utilizing a package script.
  find_package(GTest REQUIRED)

  if(GTEST_FOUND)
    set(GTEST_DIR "${GTEST_ROOT}")
  else()
    message(
      FATAL_ERROR
      "Gtest not found, not building tests. "
      "Please specify valid directory with -Dgtest_DIR="
    )
  endif()

  set(VT_HAS_GTEST TRUE)
else()
  message(
    STATUS
    "Gtest is disabled; not building tests. To enable set -Denable_gtest=true."
    )
  set(VT_HAS_GTEST FALSE)
endif()

# Threading build configuration
option(USE_STD_THREAD "whether to force use of std::thread for threading" OFF)
option(USE_OPENMP "whether to force use of OpenMP for threading" OFF)

find_package(OpenMP)

# OpenMP support
if (USE_STD_THREAD)
  message("Using std::thread for worker threading")
  config_for_std_thread()
elseif(USE_OPENMP)
  config_for_openmp()
  if (NOT OpenMP_Found)
    message(
      FATAL_ERROR
      "requested OpenMP with -DUSE_OPENMP=On, but cannot find "
      "valid OpenMP in compiler"
    )
  endif()
elseif(OpenMP_FOUND) #no default specified
  config_for_openmp()
else() #no default specified
  message("OpenMP not found: using std::thread for workers")
  config_for_std_thread()
endif()

set(PROJECT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

include (CTest)
enable_testing()

# Set default command for invoking MPI (mpirun) and flag for MPI nprocs
set(MPI_RUN_COMMAND  "${MPIEXEC_EXECUTABLE}")
set(MPI_PRE_FLAGS    "${MPIEXEC_PREFLAGS}")
set(MPI_EPI_FLAGS    "${MPIEXEC_POSTFLAGS}")
set(MPI_NUMPROC_FLAG "${MPIEXEC_NUMPROC_FLAG}")
set(MPI_MAX_NUMPROC  "${MPIEXEC_MAX_NUMPROCS}")

message(STATUS "MPI detected max nproc: ${MPIEXEC_MAX_NUMPROCS}")

set(PROC_TEST_LIST "")
build_mpi_proc_test_list(
  MAX_PROC       ${MPI_MAX_NUMPROC}
  VARIABLE_OUT   PROC_TEST_LIST
)

message(STATUS "MPI proc test list: ${PROC_TEST_LIST}")

# Add subdirectories for tests and examples
add_subdirectory(${PROJECT_LIB_DIR}/context)
add_subdirectory(src)

add_custom_target(examples)
add_subdirectory(examples)

add_custom_target(unit_tests)
add_custom_target(perf_tests)
add_subdirectory(tests)

set(
  PROJECT_PERF_TESTS
  ping_pong
)

configure_file(
  cmake/vtConfig.cmake.in
  "${PROJECT_BINARY_DIR}/vtConfig.cmake" @ONLY
)

install(
  FILES
  "${PROJECT_BINARY_DIR}/vtConfig.cmake"
  DESTINATION cmake
  COMPONENT extCfg
)

configure_file(cmake_config.h.in cmake_config.h @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/cmake_config.h" DESTINATION include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


