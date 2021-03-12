cmake_minimum_required(VERSION 3.8)
project(fcontext)

enable_language(CXX ASM)

include (CTest)
enable_testing()

EXECUTE_PROCESS(COMMAND uname -p OUTPUT_VARIABLE CPU_ARCH )
string(REGEX REPLACE "\n$" ""  CPU_ARCH "${CPU_ARCH}" )

if (CPU_ARCH MATCHES "ppc")
  set(CPU_ARCH "ppc64")
endif()

# this is cuz macs give the wrong arch
if (CPU_ARCH MATCHES "i386" )
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPU_ARCH "x86_64")
  endif()
endif()


set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(PROJECT_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

file(
  GLOB
  HEADER_FILES
  ${PROJECT_SOURCE_DIR}/context/*.h
)

file(
  GLOB
  SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/context/*.cc
)

if (APPLE)
  set(ASM_EXT "sysv_macho_gas.S") # OSX
else()
  set(ASM_EXT "sysv_elf_gas.S")  # Linux/Unix
endif()

if (CPU_ARCH MATCHES "unknown")
  set(CPU_ARCH "x86_64")
  set(ASM_EXT "sysv_elf_gas.S") # OSX
endif()

set(
  ASM_SOURCES
  "${PROJECT_SOURCE_DIR}/context/asm/make_${CPU_ARCH}_${ASM_EXT}"
  "${PROJECT_SOURCE_DIR}/context/asm/jump_${CPU_ARCH}_${ASM_EXT}"
  "${PROJECT_SOURCE_DIR}/context/asm/ontop_${CPU_ARCH}_${ASM_EXT}"
)

#message("ASM_SOURCES=${ASM_SOURCES}")

set(FCONTEXT_LIBRARY fcontext)

add_library(
  ${FCONTEXT_LIBRARY}
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES} ${ASM_SOURCES}
)

set(VT_CONTEXT_INSTALL_DESTINATION "include/context")
set(VT_CONTEXT_EXTERNAL_DESTINATION "include")

install(
  FILES        ${HEADER_FILES}
  DESTINATION  ${VT_CONTEXT_INSTALL_DESTINATION}
)

target_compile_options(${FCONTEXT_LIBRARY} PUBLIC ${CXX_STANDARD_FLAGS})
target_compile_features(${FCONTEXT_LIBRARY} PUBLIC cxx_std_14)

target_include_directories(
  ${FCONTEXT_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${VT_CONTEXT_EXTERNAL_DESTINATION}>
)

set(PROJECT_EXAMPLE_C_DIR ${PROJECT_EXAMPLE_DIR}/c_interface)
set(PROJECT_EXAMPLE_CXX_DIR ${PROJECT_EXAMPLE_DIR}/cxx_interface)

file(
  GLOB
  PROJECT_C_EXAMPLES
  RELATIVE
  "${PROJECT_EXAMPLE_C_DIR}"
  "${PROJECT_EXAMPLE_C_DIR}/*.cc"
)

file(
  GLOB
  PROJECT_CXX_EXAMPLES
  RELATIVE
  "${PROJECT_EXAMPLE_CXX_DIR}"
  "${PROJECT_EXAMPLE_CXX_DIR}/*.cc"
)

file(
  GLOB
  PROJECT_EXAMPLE_TOP_HEADERS
  RELATIVE
  "${PROJECT_EXAMPLE_DIR}"
  "${PROJECT_EXAMPLE_DIR}/*.h"
)

file(
  GLOB
  PROJECT_EXAMPLE_TOP_IMPL
  RELATIVE
  "${PROJECT_EXAMPLE_DIR}"
  "${PROJECT_EXAMPLE_DIR}/*.cc"
)

include_directories(${PROJECT_EXAMPLE_DIR})

message(STATUS "Context library: PROJECT_EXAMPLE_DIR=${PROJECT_EXAMPLE_DIR}")
# message(STATUS "PROJECT_EXAMPLE_TOP_HEADERS=${PROJECT_EXAMPLE_TOP_HEADERS}")
# message(STATUS "PROJECT_EXAMPLE_TOP_IMPL=${PROJECT_EXAMPLE_TOP_IMPL}")
# message(STATUS "PROJECT_EXAMPLE_C_DIR=${PROJECT_EXAMPLE_C_DIR}")
# message(STATUS "PROJECT_EXAMPLE_CXX_DIR=${PROJECT_EXAMPLE_CXX_DIR}")

macro(add_test_for_example test_name test_exec)
  add_test(
    ${test_name} ${test_exec} ${ARGN}
  )

  set_tests_properties(
    ${test_name}
    PROPERTIES TIMEOUT 300 FAIL_REGULAR_EXPRESSION "FAILED;WARNING"
  )
endmacro()

set(PROJECT_EXAMPLE_TOP_IMPL_FILES )
set(PROJECT_EXAMPLE_TOP_HEADER_FILES )

foreach(FILE ${PROJECT_EXAMPLE_TOP_IMPL})
  list(APPEND PROJECT_EXAMPLE_TOP_IMPL_FILES "${PROJECT_EXAMPLE_DIR}/${FILE}")
endforeach()

foreach(FILE ${PROJECT_EXAMPLE_TOP_HEADERS})
  list(APPEND PROJECT_EXAMPLE_TOP_HEADER_FILES "${PROJECT_EXAMPLE_DIR}/${FILE}")
endforeach()

option(
  vt_fcontext_build_tests_examples "Build fcontext (ULTs) tests and examples"
  OFF
)

if(vt_fcontext_build_tests_examples)

  foreach(EXAMPLE_FULL ${PROJECT_C_EXAMPLES})
    GET_FILENAME_COMPONENT(
      EXAMPLE
      ${EXAMPLE_FULL}
      NAME_WE
    )

    add_executable(
      ${EXAMPLE}
      ${PROJECT_EXAMPLE_C_DIR}/${EXAMPLE}.cc
      ${PROJECT_EXAMPLE_TOP_HEADER_FILES}
      ${PROJECT_EXAMPLE_TOP_IMPL_FILES}
    )

    add_test_for_example(
      ${EXAMPLE}
      ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}
    )

    target_link_libraries(
      ${EXAMPLE}
      ${FCONTEXT_LIBRARY}
    )
  endforeach()

  foreach(EXAMPLE_FULL ${PROJECT_CXX_EXAMPLES})
    GET_FILENAME_COMPONENT(
      EXAMPLE
      ${EXAMPLE_FULL}
      NAME_WE
    )

    add_executable(
      ${EXAMPLE}
      ${PROJECT_EXAMPLE_CXX_DIR}/${EXAMPLE}.cc
      ${PROJECT_EXAMPLE_TOP_HEADER_FILES}
      ${PROJECT_EXAMPLE_TOP_IMPL_FILES}
    )

    add_test_for_example(
      ${EXAMPLE}
      ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}
    )

    target_link_libraries(
      ${EXAMPLE}
      ${FCONTEXT_LIBRARY}
    )
  endforeach()

endif()

install(
  TARGETS                   ${FCONTEXT_LIBRARY}
  EXPORT                    ${VIRTUAL_TRANSPORT_LIBRARY}
  COMPONENT                 context
  LIBRARY DESTINATION       lib
  ARCHIVE DESTINATION       lib
  RUNTIME DESTINATION       bin
  INCLUDES DESTINATION      ${VT_CONTEXT_EXTERNAL_DESTINATION}
)
