# Compiler-specific C++14 activation.
# Call this from all CMakeLists.txt files that can be built independently.

macro(set_darma_compiler_flags)

set(CXX_STANDARD_FLAGS)
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # 4.9.3 complains about std::min not being constexpr
  set(CXX_STANDARD_FLAGS -std=c++14)
  if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 5))
    message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CXX_STANDARD_FLAGS -std=c++1y -ftemplate-depth=900)
  if (APPLE)
    list(APPEND CXX_STANDARD_FLAGS -stdlib=libc++ -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7)
    list(APPEND CXX_STANDARD_FLAGS -DCLI11_EXPERIMENTAL_OPTIONAL=0)
  endif()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    list(APPEND CXX_STANDARD_FLAGS -Wno-missing-braces)
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # 16.0.3 complains about std::min not being constexpr
  set(${CXX_STANDARD_FLAGS} -std=c++14)
else ()
  message(FATAL_ERROR "Your C++ compiler may not support C++14.")
endif ()

endmacro()


