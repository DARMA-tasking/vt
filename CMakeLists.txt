cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(vt)

include(cmake/turn_on_warnings.cmake)

include(cmake/check_system_functions.cmake)

set(VIRTUAL_TRANSPORT_LIBRARY vt CACHE INTERNAL "" FORCE )
set(FCONTEXT_LIBRARY fcontext)

# Set the local module path so custom cmake scripts can be located automatically
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/")

# Try to find ccache to speed up compilation
find_program(ccache_binary ccache)
if (ccache_binary)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${ccache_binary}")
endif()

include(SetCXXCompilerFlags)
set_darma_compiler_flags()

# Import the linking macros for VT-related targets
include(cmake/link_vt.cmake)

# Load all the packages that are required for VT
include(cmake/load_packages.cmake)

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

include(cmake/build_git_info.cmake)

option(vt_fcontext_enabled "Build VT with fcontext (ULT) enabled" OFF)

# Add subdirectories for tests and examples
if (${vt_fcontext_enabled})
  add_subdirectory(${PROJECT_LIB_DIR}/context)
endif()
add_subdirectory(${PROJECT_LIB_DIR}/CLI)
add_subdirectory(${PROJECT_LIB_DIR}/fmt)

option(vt_mimalloc_enabled "Build VT with mimalloc" OFF)
option(vt_mimalloc_static "Build VT with mimalloc using static linking" ON)
if (${vt_mimalloc_enabled})
  add_subdirectory(${PROJECT_LIB_DIR}/mimalloc)
  if (${vt_mimalloc_static})
    set(MIMALLOC_LIBRARY mimalloc-static)
  else()
    set(MIMALLOC_LIBRARY mimalloc)
  endif()
endif()

# Primary VT build
add_subdirectory(src)

add_custom_target(examples)
add_subdirectory(examples)

add_custom_target(unit_tests)
add_custom_target(perf_tests)
add_subdirectory(tests)

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
