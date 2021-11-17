# Call this from all CMakeLists.txt files that can be built independently.

macro(set_darma_compiler_flags vt_target)

set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # 4.9.3 complains about std::min not being constexpr
  if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 5))
    message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    list(APPEND TARGET_CXX_FLAGS -Wno-unused-variable)
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  list(APPEND TARGET_CXX_FLAGS -ftemplate-depth=900)
  if (APPLE)
    list(APPEND TARGET_CXX_FLAGS -stdlib=libc++ -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7)
    list(APPEND TARGET_CXX_FLAGS -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    list(APPEND TARGET_CXX_FLAGS -Wno-missing-braces)
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL IntelLLVM AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 2021.3.0)
  list(APPEND TARGET_CXX_FLAGS -fhonor-infinites -fhonor-nans)
elseif (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  message(FATAL_ERROR "Your C++ compiler may not support C++14.")
endif ()

if (vt_asan_enabled)
  list(APPEND TARGET_CXX_FLAGS -fsanitize=address -fno-omit-frame-pointer)
endif()

if (vt_ubsan_enabled)
  add_definitions(-DVT_UBSAN_ENABLED)
  list(APPEND TARGET_CXX_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
endif()

message(DEBUG "Target ${vt_target} compile options: ${TARGET_CXX_FLAGS}")
target_compile_options(${vt_target} PUBLIC ${TARGET_CXX_FLAGS})

endmacro()
