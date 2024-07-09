# Set minimum CMake version
cmake_minimum_required(VERSION 3.23 FATAL_ERROR)


# Find the PAPI include directory and library
message(STATUS "Finding PAPI, PAPI_BUILD_ROOT: ${papi_ROOT}")
find_path(PAPI_INCLUDE_DIR NAMES papi.h HINTS ${papi_ROOT}/include)
find_library(PAPI_LIBRARY NAMES papi HINTS ${papi_ROOT}/lib)

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
