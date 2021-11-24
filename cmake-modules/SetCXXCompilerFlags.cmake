# Call this from all CMakeLists.txt files that can be built independently.

macro(set_darma_compiler_flags vt_target)

set(CMAKE_CXX_EXTENSIONS OFF)
if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  # 4.9.3 complains about std::min not being constexpr
  if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5))
    message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
  endif ()
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  list(APPEND TARGET_PUBLIC_CXX_FLAGS -ftemplate-depth=900)
  if (APPLE)
    list(APPEND TARGET_PUBLIC_CXX_FLAGS -stdlib=libc++ -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7)
    list(APPEND TARGET_PUBLIC_CXX_FLAGS -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL IntelLLVM AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 2021.3.0)
  list(APPEND TARGET_PRIVATE_CXX_FLAGS -fhonor-infinites -fhonor-nans)
elseif (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
  message(FATAL_ERROR "Your C++ compiler may not support C++14.")
endif ()

if (vt_asan_enabled)
  list(APPEND TARGET_PUBLIC_CXX_FLAGS -fsanitize=address -fno-omit-frame-pointer)
endif()

if (vt_ubsan_enabled)
  add_definitions(-DVT_UBSAN_ENABLED)
  list(APPEND TARGET_PUBLIC_CXX_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
endif()

message(DEBUG "Target ${vt_target} public compile options: ${TARGET_CXX_FLAGS}")
target_compile_options(${vt_target} PUBLIC ${TARGET_PUBLIC_CXX_FLAGS})

message(DEBUG "Target ${vt_target} private compile options: ${TARGET_CXX_FLAGS}")
target_compile_options(${vt_target} PRIVATE ${TARGET_PRIVATE_CXX_FLAGS})

endmacro()
