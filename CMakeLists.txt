cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

file(READ "VERSION" _vt_version_str)
string(STRIP "${_vt_version_str}" _vt_version_str)
project(vt VERSION ${_vt_version_str})

# To generate output file with compilation errors and warnings
# CMake generator needs to be known
set(USED_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING "Expose CMAKE_GENERATOR" FORCE)

# Prevent mixing install policies from add_subdirectory;
# This suppresses a warning as the old behavior is deprecated-by-definition.
cmake_policy(SET CMP0082 NEW)

get_directory_property(hasParent PARENT_DIRECTORY)

include(cmake/check_system_functions.cmake)

set(VIRTUAL_TRANSPORT_LIBRARY vt CACHE INTERNAL "" FORCE )

# Set the local module path so custom cmake scripts can be located automatically
set(
  CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/"
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
  # Try to find ccache to speed up compilation
  find_program(ccache_binary ccache)
  if (ccache_binary)
    message(
      STATUS
      "VT: Found ccache binary: ${ccache_binary}; adding launch rule"
    )
    set(CMAKE_CXX_COMPILER_LAUNCHER ${ccache_binary})
  endif()
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
endif()
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

set(MPI_EXTRA_FLAGS "" CACHE STRING "Flags to pass to mpirun/mpiexec")
string(REPLACE " " ";" MPI_EXTRA_FLAGS_LIST "${MPI_EXTRA_FLAGS}")
message(STATUS "Running tests and examples with additional MPI flags = ${MPI_EXTRA_FLAGS_LIST}")


set(PROJECT_BIN_DIR     ${CMAKE_CURRENT_BINARY_DIR})
set(PROJECT_BASE_DIR    ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_LIB_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(PROJECT_TOOLS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Import the linking macros for VT-related targets
include(cmake/link_vt.cmake)

# Load packages that are required for core VT build
include(cmake/load_packages.cmake)

# Some options use TPL information (whether Perl was found etc.)
include(cmake/configure_options.cmake)

include(cmake/check_compiler.cmake)
include(cmake/nvcc_no_deprecated_gpu_targets.cmake)
include(cmake/load_bundled_libraries.cmake)
include(cmake/load_local_packages.cmake)

# Primary VT build
add_subdirectory(src)

if(vt_no_color_enabled)
  target_compile_definitions(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC VT_NO_COLOR_ENABLED)
endif()
#
# Test prep - ensures examples can be registered as tests.
#
if (vt_build_tests
   AND "${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  # CTest implies enable_testing() and defines the BUILD_TESTING option.
  # The default of BUILD_TESTING is ON.
  # Testing is only enabled if the actual project being built is VT.
  include(CTest)
endif()

#
# Tools
#
if (vt_build_tools)
  message(
    STATUS
    "VT: building tools"
  )

  add_custom_target(tools)
  add_subdirectory(tools)
else()
  message(
    STATUS "VT: NOT building tools because vt_build_tools is not set."
  )
endif()

#
# Examples
#
if (vt_build_examples)
  message(
    STATUS
    "VT: building examples"
  )

  add_custom_target(examples)
  add_subdirectory(examples)
else()
  message(
    STATUS "VT: NOT building examples because vt_build_examples is not set.\
 Examples that are not built are NOT TESTED."
  )
endif()

#
# Tests
#

if (BUILD_TESTING
   AND vt_build_tests
   AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  message(
    STATUS
    "VT: building tests"
  )

  add_custom_target(unit_tests)
  add_custom_target(perf_tests)
  add_subdirectory(tests)
else()
  message(
    STATUS
    "VT: NOT building tests because vt_build_tests or BUILD_TESTING are not set"
  )
endif()

# Configure file for the VT package
configure_file(
  cmake/vtConfig.cmake.in
  "${PROJECT_BINARY_DIR}/vtConfig.cmake" @ONLY
)

install(
  FILES        "${PROJECT_BINARY_DIR}/vtConfig.cmake"
  DESTINATION  cmake
  COMPONENT    runtime
)
