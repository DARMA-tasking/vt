cmake_minimum_required(VERSION 3.8)
project(fcontext)

set(CMAKE_CXX_STANDARD 14)

enable_language(CXX ASM)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

file(
  GLOB
  HEADER_FILES
  ${PROJECT_SOURCE_DIR}/*.h
)

file(
  GLOB
  SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/*.cc
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CPU_ARCH "x86_64")
else()
  set(CPU_ARCH "i386")
endif()

if (APPLE)
  set(ASM_EXT "sysv_macho_gas.S") # OSX
else()
  set(ASM_EXT "sysv_elf_gas.S")  # Linux/Unix
endif()

set(
  ASM_SOURCES
  "${PROJECT_SOURCE_DIR}/asm/make_${CPU_ARCH}_${ASM_EXT}"
  "${PROJECT_SOURCE_DIR}/asm/jump_${CPU_ARCH}_${ASM_EXT}"
  "${PROJECT_SOURCE_DIR}/asm/ontop_${CPU_ARCH}_${ASM_EXT}"
)

#message("ASM_SOURCES=${ASM_SOURCES}")

set(FCONTEXT_LIBRARY fcontext)

add_library(
  ${FCONTEXT_LIBRARY}
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES} ${ASM_SOURCES}
)

install(TARGETS ${FCONTEXT_LIBRARY} DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(
  test_fcontext
  ${PROJECT_TEST_DIR}/test_fcontext.cc
)

target_link_libraries(
  test_fcontext
  ${FCONTEXT_LIBRARY}
)
