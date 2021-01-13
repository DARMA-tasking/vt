cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vt)

if (POLICY CMP0082) # CMake 3.14
  # Prevent mixing install policies from add_subdirectory;
  # This suppresses a warning as the old behavior is deprecated-by-definition.
  cmake_policy(SET CMP0082 NEW)
endif()

include(cmake/turn_on_warnings.cmake)

include(cmake/check_system_functions.cmake)

set(VIRTUAL_TRANSPORT_LIBRARY vt CACHE INTERNAL "" FORCE )
set(FCONTEXT_LIBRARY fcontext)

# Set the local module path so custom cmake scripts can be located automatically
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/")

# Try to find ccache to speed up compilation
find_program(ccache_binary ccache)
if (ccache_binary)
  message(
    STATUS
    "VT: Found ccache binary: ${ccache_binary}; adding launch rule"
  )
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${ccache_binary}")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()

include(SetCXXCompilerFlags)
set_darma_compiler_flags()

# Code coverage option of VT
option(CODE_COVERAGE "Enable coverage reporting" OFF)
# OPTION(CODE_COVERAGE_ENABLED FALSE)
# if(CODE_COVERAGE_ENABLED)
#     include(cmake/code_coverage.cmake)
# endif(CODE_COVERAGE_ENABLED)

set(MPI_EXTRA_FLAGS "" CACHE STRING "Flags to pass to mpirun/mpiexec")

set(PROJECT_BIN_DIR     ${CMAKE_CURRENT_BINARY_DIR})
set(PROJECT_BASE_DIR    ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_LIB_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

# Import the linking macros for VT-related targets
include(cmake/link_vt.cmake)

# Load packages that are required for core VT build
include(cmake/load_packages.cmake)

# Request C++14, allow fallback (to C++11).. default is to fallback.
# Might be ideal to have builds request a lower standard.
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/build_git_info.cmake)

include(cmake/check_compiler.cmake)

option(vt_gold_linker_enabled "Build VT using the `gold' linker" ON)

option(vt_unity_build_enabled "Build VT with Unity/Jumbo mode enabled" OFF)

if (vt_unity_build_enabled)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    message(STATUS "Building VT in Unity/Jumbo build mode")
  else()
    message(
      STATUS
      "Could *not* build VT in Unity/Jumbo mode---version of CMake ${CMAKE_VERSION} < 3.16"
    )
  endif()
endif()

option(vt_fcontext_enabled "Build VT with fcontext (ULT) enabled" OFF)

# Option included here, handled in define_build_modes.cmake so libfort can be
# included correspondingly
option(vt_diagnostics_enabled "Build VT with performance metrics/stats" ON)

if (vt_diagnostics_enabled)
  # Default libfort ON if diagnostics are enabled
  option(vt_libfort_enabled "Build VT with fort library enabled" ON)
else()
  option(vt_libfort_enabled "Build VT with fort library enabled" OFF)
endif()

option(vt_fcontext_enabled "Build VT with fcontext (ULT) enabled" OFF)
option(vt_mimalloc_enabled "Build VT with mimalloc" OFF)
option(vt_mimalloc_static "Build VT with mimalloc using static linking" ON)
option(vt_asan_enabled "Build VT with address sanitizer" OFF)

option(vt_werror_enabled "Build VT with -Werror enabled" OFF)
if (vt_werror_enabled)
  enable_cxx_compiler_flag_if_supported("-Werror")
endif()
include(cmake/load_bundled_libraries.cmake)

# Primary VT build
add_subdirectory(src)

#
# Test prep - ensures examples can be registered as tests.
#
option(VT_BUILD_TESTS "Build VT tests" ON)

if (VT_BUILD_TESTS
   AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  # CTest implies enable_testing() and defines the BUILD_TESTING option.
  # The default of BUILD_TESTING is ON.
  # Testing is only enabled if the actual project being built is VT.
  include(CTest)
endif()

#
# Examples
#
option(VT_BUILD_EXAMPLES "Build VT examples" ON)

if (VT_BUILD_EXAMPLES)
  message(
    STATUS
    "VT: building examples"
  )

  add_custom_target(examples)
  add_subdirectory(examples)
else()
  message(
    STATUS "VT: NOT building examples because VT_BUILD_EXAMPLES is not set.\
 Examples that are not built are NOT TESTED."
  )
endif()

#
# Tests
#

if (BUILD_TESTING
   AND VT_BUILD_TESTS
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
    "VT: NOT building tests because VT_BUILD_TESTS or BUILD_TESTING are not set"
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
