cmake_minimum_required(VERSION 3.8)
project(vt)

set(VIRTUAL_TRANSPORT_LIBRARY vt)
set(FCONTEXT_LIBRARY fcontext)

# Set the local module path so custom cmake scripts can be located automatically
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules/")

include(SetCXXCompilerFlags)
set_darma_compiler_flags()

macro(require_pkg_directory pkg_name pkg_user_name)
  #message(STATUS "require_directory: name=${pkg_name}")
  option(${pkg_name}_DIR "Root folder for ${pkg_user_name} installation" OFF)
  if (NOT ${pkg_name}_DIR)
    message(
      FATAL_ERROR
      "Please specify ${pkg_user_name} library installation root"
      " with -D${pkg_name}_DIR="
    )
  endif()
endmacro(require_pkg_directory)

macro(find_package_local pkg_name pkg_directory)
  message(
    STATUS
    "find_package_local: "
    "pkg name=\"${pkg_name}\", directory=\"${pkg_directory}\""
  )
  # search locally only for package
  find_package(
    ${pkg_name}
    PATHS ${pkg_directory}
    REQUIRED
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_BUILDS_PATH
    NO_CMAKE_SYSTEM_PATH
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
  )
endmacro(find_package_local)

# require directories for these packages
require_pkg_directory(darma_meld     "DARMA meld")
require_pkg_directory(darma_detector "DARMA detector")
# find these required packages locally
find_package_local(darma_meld     "${darma_meld_DIR}/cmake")
find_package_local(darma_detector "${darma_detector_DIR}/cmake")

macro(optional_pkg_directory pkg_name pkg_user_name)
  #message(STATUS "optional_pkg_directory: name=${pkg_name}")
  option(${pkg_name}_DIR "Root folder for ${pkg_user_name} installation" OFF)
  if (NOT ${pkg_name}_DIR)
    message(
      STATUS
      "Path for ${pkg_user_name} library (optional) not specified "
      "with -D${pkg_name}_DIR="
    )
    message(
      STATUS
      "Building without ${pkg_user_name} library"
    )
    set(${pkg_name}_DIR_FOUND 0)
  else()
    message(
      STATUS
      "Path for ${pkg_user_name} library (optional) specified "
      "with -D${pkg_name}_DIR=${${pkg_name}_DIR}"
    )
    set(${pkg_name}_DIR_FOUND 1)
  endif()
endmacro(optional_pkg_directory)

# optional directory for this package
optional_pkg_directory(darma_checkpoint "Serialization/Checkpoint")
# if it is found then find the package
if (${darma_checkpoint_DIR_FOUND})
  find_package_local(darma_checkpoint "${darma_checkpoint_DIR}/cmake")
  if (${darma_checkpoint_FOUND})
    set(CMAKE_HAS_SERIALIZATION_LIBRARY 1)
    set(SERIALIZATION_LIBRARY darma_checkpoint)
  else()
    message(FATAL_ERROR "Serialization/checkpoint library not found")
  endif()
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

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message("zlib is required for tracing")
endif (ZLIB_FOUND)

find_package(fmt PATHS ${fmt_DIR} ${fmt_DIR}/lib/cmake)
if(NOT fmt_FOUND)
  message(
    FATAL_ERROR
    "VT requires the fmt library. Please specify valid installation with "
    "-Dfmt_DIR= or install by downloading from https://github.com/fmtlib/fmt"
  )
endif()

function(config_for_std_thread)
  set(DEFAULT_THREADING stdthread)
endfunction(config_for_std_thread)

find_package(OpenMP)

function(config_for_openmp)
  set(DEFAULT_THREADING openmp)
  message(
    STATUS
    "OpenMP has been found: "
    "Linker=\"${OpenMP_EXE_LINKER_FLAGS}\", "
    "CC FLAGS=\"${OpenMP_C_FLAGS}\", "
    "CXX FLAGS=\"${OpenMP_CXX_FLAGS}\""
  )

  #
  # The OpenMP compiler and linker flags are handled through the target instead
  # of manually setting the flags for VT:
  #
  # string(APPEND VT_TARGET_CXX_FLAGS ${OpenMP_CXX_FLAGS})
  # set(VT_TARGET_CXX_FLAGS "${VT_TARGET_CXX_FLAGS}" PARENT_SCOPE)
  # string(APPEND VT_TARGET_LINKER_FLAGS ${OpenMP_EXE_LINKER_FLAGS})
  # set(VT_TARGET_LINKER_FLAGS "${VT_TARGET_LINKER_FLAGS}" PARENT_SCOPE)
endfunction(config_for_openmp)

option(USE_STD_THREAD "whether to force use of std::thread for threading" OFF)
option(USE_OPENMP "whether to force use of OpenMP for threading" OFF)

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
set(MPI_RUN_COMMAND "mpirun-mpich-clang")
set(MPI_NUMPROC_FLAG "-n")
set(PROC_TEST_LIST 2 4 8)

# Set up test scaffolding for running examples
macro(add_test_for_example_vt test_name test_exec)
  foreach(PROC ${PROC_TEST_LIST})
    if(CMAKE_NO_BUILD_TESTS)
      message("Skipping test: ${test_name}_${PROC}")
    else()
      #message("Adding test for example: ${test_name}")
      add_test(
        ${test_name}_${PROC}
        ${MPI_RUN_COMMAND} ${MPI_NUMPROC_FLAG} ${PROC}
        ${test_exec} ${ARGN}
      )

      set_tests_properties(
        ${test_name}_${PROC}
        PROPERTIES TIMEOUT 300 FAIL_REGULAR_EXPRESSION "FAILED;WARNING"
      )
    endif()
  endforeach()
endmacro()

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


