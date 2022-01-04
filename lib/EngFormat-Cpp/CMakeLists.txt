project(EngFormat-Cpp CXX)
cmake_minimum_required(VERSION 3.0)

add_library(${PROJECT_NAME} include/EngFormat-Cpp/eng_format.hpp src/eng_format.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  if(BUILD_TESTING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENG_FORMAT_MICRO_GLYPH="u")
    add_subdirectory(test)
  endif()
endif()
