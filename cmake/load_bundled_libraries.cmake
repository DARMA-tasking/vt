
# Bundled dependencies

# Export a minimum version flag for any bundled libraries that don't set their own
set(CMAKE_CXX_STANDARD 14)

# Optionally include libfort which is used by diagnostics
if (vt_libfort_enabled)
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
set(FMT_LIBRARY fmt)
add_subdirectory(${PROJECT_LIB_DIR}/fmt)

# json library always included in the build
set(JSON_BuildTests OFF)
set(JSON_MultipleHeaders ON)
set(JSON_LIBRARY nlohmann_json)
add_subdirectory(${PROJECT_LIB_DIR}/json)

# brotli library always included in the build
set(BROTLI_DISABLE_TESTS ON)
# we need to disable bundled mode so it will install properly
set(BROTLI_BUNDLED_MODE OFF)
set(BROTLI_BUILD_PORTABLE ON)
set(BROTLI_LIBRARY brotlicommon-static brotlienc-static brotlidec-static)
add_subdirectory(${PROJECT_LIB_DIR}/brotli)

# Optionally include mimalloc (alternative memory allocator)
if (vt_mimalloc_enabled)
  add_subdirectory(${PROJECT_LIB_DIR}/mimalloc)
  if (${vt_mimalloc_static})
    set(MIMALLOC_LIBRARY mimalloc-static)
  else()
    set(MIMALLOC_LIBRARY mimalloc)
  endif()
endif()

# Check if address sanitizer can be enabled
if (vt_asan_enabled)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Building with address sanitizer enabled")
  endif()
endif()
