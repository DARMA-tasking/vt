
# Bundled dependencies

# Optionally include libfort which is used by diagnostics
if (vt_fort_enabled)
  set(FORT_ENABLE_TESTING OFF CACHE INTERNAL "")
  add_subdirectory(${PROJECT_LIB_DIR}/libfort)
  set(FORT_LIBRARY fort)
endif()

# Optionally include fcontext
if (vt_fcontext_enabled)
  add_subdirectory(${PROJECT_LIB_DIR}/context)
endif()

# CLI11 always included in the build
add_subdirectory(${PROJECT_LIB_DIR}/CLI)

# fmt always included in the build
add_subdirectory(${PROJECT_LIB_DIR}/fmt)

# Optionally include mimalloc (alternative memory allocator)
if (vt_mimalloc_enabled)
  add_subdirectory(${PROJECT_LIB_DIR}/mimalloc)
  if (${vt_mimalloc_static})
    set(MIMALLOC_LIBRARY mimalloc-static)
  else()
    set(MIMALLOC_LIBRARY mimalloc)
  endif()
endif()

# Optionally enable address sanitizer library in build
if (vt_asan_enabled)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     (CMAKE_CXX_COMPILER_ID
       STREQUAL
       "GNU"
       AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.8"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -DADDRESS_SANITIZER")
    message(STATUS "Building with address sanitizer enabled")
  else()
    message(SEND_ERROR "Cannot use ASAN without clang or gcc >= 4.8")
  endif()
endif()
