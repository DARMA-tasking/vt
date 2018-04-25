cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(vt)

set(VIRTUAL_TRANSPORT_LIBRARY vt)
set(FCONTEXT_LIBRARY fcontext)

# Set the local module path so custom cmake scripts can be located automatically
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules/")

include(SetCXXCompilerFlags)
set_darma_compiler_flags()

# Import the linking macros for VT-related targets
include(cmake/link_vt.cmake)

# Load all the packages that are required for VT
include(cmake/load_packages.cmake)

set(PROJECT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

# Add subdirectories for tests and examples
add_subdirectory(${PROJECT_LIB_DIR}/context)
add_subdirectory(src)

add_custom_target(examples)
add_subdirectory(examples)

add_custom_target(unit_tests)
add_custom_target(perf_tests)
add_subdirectory(tests)

configure_file(
  cmake/vtConfig.cmake.in
  "${PROJECT_BINARY_DIR}/vtConfig.cmake" @ONLY
)

install(
  FILES        "${PROJECT_BINARY_DIR}/vtConfig.cmake"
  DESTINATION  cmake
  COMPONENT    runtime
)

configure_file(cmake_config.h.in cmake_config.h @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/cmake_config.h" DESTINATION include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


