project(FMT CXX)

# Joins arguments and places the results in ${result_var}.
function(join result_var)
  set(result )
  foreach (arg ${ARGN})
    set(result "${result}${arg}")
  endforeach ()
  set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

# Get version from core.h
file(READ include/fmt/core.h core_h)
if (NOT core_h MATCHES "FMT_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])")
  message(FATAL_ERROR "Cannot get FMT_VERSION from core.h.")
endif ()
# Use math to skip leading zeros if any.
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
join(FMT_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})
message(STATUS "fmt version: ${FMT_VERSION}")

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/fmt/${header})
  endforeach()
  set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

# Define the fmt library, its includes and the needed defines.
add_headers(FMT_HEADERS chrono.h color.h compile.h core.h format.h format-inl.h
                        locale.h ostream.h printf.h ranges.h)
set(FMT_SOURCES src/format.cc)

add_library(fmt ${FMT_SOURCES} ${FMT_HEADERS})
add_library(fmt::fmt ALIAS fmt)

target_include_directories(fmt PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(FMT_DEBUG_POSTFIX d CACHE STRING "Debug library postfix.")

set_target_properties(fmt PROPERTIES
  VERSION ${FMT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}
  DEBUG_POSTFIX "${FMT_DEBUG_POSTFIX}")

install(
  DIRECTORY                include/fmt
  DESTINATION              include
  CONFIGURATIONS           ${build_type_list}
  FILES_MATCHING PATTERN   "*"
)
