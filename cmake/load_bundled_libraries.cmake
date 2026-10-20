# Bundled dependencies

include(SetCXXCompilerFlags)

# Optionally include libfort which is used by diagnostics
if (vt_libfort_enabled)
  set(FORT_ENABLE_TESTING OFF CACHE INTERNAL "")
  add_subdirectory(${PROJECT_LIB_DIR}/libfort)
  set(FORT_LIBRARY fort)
  set_darma_compiler_flags(${FORT_LIBRARY})
endif()

# Optionally include fcontext
if (vt_fcontext_enabled)
  set(FCONTEXT_LIBRARY fcontext)
  add_subdirectory(${PROJECT_LIB_DIR}/context)
  set_darma_compiler_flags(${FCONTEXT_LIBRARY})
endif()

# CLI11 always included in the build
add_subdirectory(${PROJECT_LIB_DIR}/CLI)

# Use an external fmt package or configure VT's shared DARMA fmt provider.
include(setup_fmt)
if(vt_external_fmt AND NOT TARGET fmt::fmt)
  find_package(fmt 12.2.0 EXACT CONFIG REQUIRED)
  # Record the verified package version for setup_fmt's existing reuse check.
  set_property(TARGET fmt::fmt PROPERTY INTERFACE_DARMA_FMT_VERSION 120200)
endif()
darma_setup_fmt(
  SOURCE_DIR         "${PROJECT_LIB_DIR}/fmt"
  VENDORED_TARGET    fmt
  OUT_LIBRARY        FMT_LIBRARY
  OUT_LINK_TARGET    FMT_LINK_TARGET
  POST_ADD_COMMAND set_darma_compiler_flags
)
if(FMT_LIBRARY OR vt_external_fmt)
  set(vt_uses_parent_fmt 0)
else()
  set(vt_uses_parent_fmt 1)
endif()

# yaml-cpp always included in the build
set(YAMLCPP_LIBRARY yaml-cpp)
if(NOT TARGET ${YAMLCPP_LIBRARY})
  add_subdirectory(${PROJECT_LIB_DIR}/yaml-cpp)
  set_darma_compiler_flags(${YAMLCPP_LIBRARY})
endif()

# EngFormat-Cpp always included in the build
set(ENG_FORMAT_LIBRARY EngFormat-Cpp)
add_subdirectory(${PROJECT_LIB_DIR}/EngFormat-Cpp)
set_darma_compiler_flags(${ENG_FORMAT_LIBRARY})

# json library always included in the build
set(JSON_BuildTests OFF)
set(JSON_MultipleHeaders ON)
set(JSON_LIBRARY nlohmann_json-vt)
add_subdirectory(${PROJECT_LIB_DIR}/json)

# brotli library always included in the build
set(BROTLI_DISABLE_TESTS ON)
# we need to disable bundled mode so it will install properly
set(BROTLI_BUNDLED_MODE OFF)
set(BROTLI_BUILD_PORTABLE ON)
set(BROTLI_BUILD_SHARED_LIBS OFF)
set(BROTLI_LIBRARY brotlicommon-static brotlidec-static brotlienc-static)
add_subdirectory(${PROJECT_LIB_DIR}/brotli)
foreach(lib ${BROTLI_LIBRARY})
    set_darma_compiler_flags(${lib})
endforeach()

# Optionally include mimalloc (alternative memory allocator)
if (vt_mimalloc_enabled)
  add_subdirectory(${PROJECT_LIB_DIR}/mimalloc)
  if (${vt_mimalloc_static})
    set(MIMALLOC_LIBRARY mimalloc-static)
  else()
    set(MIMALLOC_LIBRARY mimalloc)
  endif()
  set_darma_compiler_flags(${MIMALLOC_LIBRARY})
endif()

# Check if sanitizers can be enabled
if (vt_asan_enabled)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Building VT with AddressSanitizer enabled")
  endif()
endif()

if (vt_ubsan_enabled)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Building VT with UndefinedBehaviorSanitizer enabled")
  else()
    message(SEND_ERROR "Cannot use UBSan without clang or gcc >= 4.8")
  endif()
endif()
