# Set minimum CMake version
cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

set(PAPI_ROOT "${CMAKE_SOURCE_DIR}/lib/papi/install")

# Find the PAPI include directory and library
find_path(PAPI_INCLUDE_DIR NAMES papi.h HINTS ${PAPI_ROOT}/include)
find_library(PAPI_LIBRARY NAMES papi HINTS ${PAPI_ROOT}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAPI DEFAULT_MSG PAPI_LIBRARY PAPI_INCLUDE_DIR)

if(PAPI_FOUND AND NOT TARGET PAPI::PAPI)
    add_library(PAPI::PAPI UNKNOWN IMPORTED)
    set_target_properties(PAPI::PAPI PROPERTIES
        IMPORTED_LOCATION "{PAPI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PAPI_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(PAPI_INCLUDE_DIR PAPI_LIBRARY)
