
# Users can pass LIBUNWIND_ROOT, LIBUNWIND_INCLUDE_DIR, and LIBUNWIND_LIBRARY as CMake variables.
# If LIBUNWIND_ROOT is provided, the INCLUDE_DIR and LIBRARY variables may be omitted.
#
# LIBUNWIND_FOUND, LIBUNWIND_INCLUDE_DIRS, and LIBUNWIND_LIBRARIES are outputs

if(LIBUNWIND_ROOT)
    set(LIBUNWIND_INCLUDE_DIR ${LIBUNWIND_INCLUDE_DIR};${LIBUNWIND_ROOT}/include)
    set(LIBUNWIND_LIBRARY ${LIBUNWIND_LIBRARY};${LIBUNWIND_ROOT}/lib64;${LIBUNWIND_ROOT}/lib)
endif()

# First, check only in the hinted paths
find_path(LIBUNWIND_INCLUDE_DIRS NAMES libunwind.h
    DOC "The libunwind include directory"
    HINTS ${LIBUNWIND_INCLUDE_DIR}
    NO_DEFAULT_PATH
)
find_library(LIBUNWIND_LIBRARIES NAMES unwind
    DOC "The libunwind library"
    HINTS ${LIBUNWIND_LIBRARY}
    NO_DEFAULT_PATH
)

# If that fails, check in CMake's default paths
find_path(LIBUNWIND_INCLUDE_DIRS NAMES libunwind.h
    DOC "The libunwind include directory"
)
find_library(LIBUNWIND_LIBRARIES NAMES unwind
    DOC "The libunwind library"
)

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUNWIND
                                  REQUIRED_VARS LIBUNWIND_LIBRARIES LIBUNWIND_INCLUDE_DIRS)

if(LIBUNWIND_FOUND)
  if(NOT TARGET libunwind)
    add_library(libunwind UNKNOWN IMPORTED)
    set_target_properties(libunwind PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBUNWIND_INCLUDE_DIRS}")
    set_property(TARGET libunwind APPEND PROPERTY IMPORTED_LOCATION "${LIBUNWIND_LIBRARY}")
  endif()
endif()
