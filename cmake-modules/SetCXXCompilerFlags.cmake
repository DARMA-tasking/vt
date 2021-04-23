# Compiler-specific C++14 activation.
# Call this from all CMakeLists.txt files that can be built independently.

macro(set_darma_compiler_flags)

set_property(DIRECTORY PROPERTY CXX_STANDARD 14)
set(CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # 4.9.3 complains about std::min not being constexpr
  if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 5))
    message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=900")
  if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -DCLI11_EXPERIMENTAL_OPTIONAL=0")
  endif ()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCLI11_EXPERIMENTAL_OPTIONAL=0")
  endif()
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
  endif()
elseif (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  message(FATAL_ERROR "Your C++ compiler may not support C++14.")
endif ()

endmacro()
