
# Users can pass libunwind_ROOT, libunwind_INCLUDE_DIR, and libunwind_LIBRARY as CMake variables.
# If libunwind_ROOT is provided, the INCLUDE_DIR and LIBRARY variables may be omitted.
#
# libunwind_FOUND, libunwind_INCLUDE_DIRS, and libunwind_LIBRARIES are outputs

if(libunwind_ROOT)
    set(libunwind_INCLUDE_DIR ${libunwind_INCLUDE_DIR};${libunwind_ROOT}/include)
    set(libunwind_LIBRARY ${libunwind_LIBRARY};${libunwind_ROOT}/lib64;${libunwind_ROOT}/lib)
endif()

# First, check only in the hinted paths
find_path(libunwind_INCLUDE_DIRS NAMES libunwind.h
    DOC "The libunwind include directory"
    HINTS ${libunwind_INCLUDE_DIR}
    NO_DEFAULT_PATH
)
find_library(libunwind_LIBRARIES NAMES unwind
    DOC "The libunwind library"
    HINTS ${libunwind_LIBRARY}
    NO_DEFAULT_PATH
)

# If that fails, check in CMake's default paths
find_path(libunwind_INCLUDE_DIRS NAMES libunwind.h
    DOC "The libunwind include directory"
)
find_library(libunwind_LIBRARIES NAMES unwind
    DOC "The libunwind library"
)

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libunwind
                                  REQUIRED_VARS libunwind_LIBRARIES libunwind_INCLUDE_DIRS)

if(libunwind_FOUND)
  if(NOT TARGET libunwind)
    add_library(libunwind UNKNOWN IMPORTED)
    set_target_properties(libunwind PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${libunwind_INCLUDE_DIRS}")
    set_property(TARGET libunwind APPEND PROPERTY IMPORTED_LOCATION "${libunwind_LIBRARY}")
  endif()
endif()
