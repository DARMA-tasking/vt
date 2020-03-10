cmake_minimum_required(VERSION 3.8)
project(memchecker)

enable_language(C)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# file(
#   GLOB
#   HEADER_FILES
#   ${PROJECT_SOURCE_DIR}/memchecker/*.h
# )

# file(
#   GLOB
#   SOURCE_FILES
#   ${PROJECT_SOURCE_DIR}/*.c
# )

# message("SOURCES=${SOURCE_FILES}")
# message("HEADERS=${HEADER_FILES}")

set(MEMCHECKER_LIBRARY memchecker-static)

add_library(
  ${MEMCHECKER_LIBRARY}
  STATIC
  memchecker-static.h memchecker-static.c
)

target_link_libraries(${MEMCHECKER_LIBRARY} PUBLIC mimalloc-static)

set(VT_CONTEXT_EXTERNAL_DESTINATION "include")

install(
  TARGETS                   ${MEMCHECKER_LIBRARY}
  EXPORT                    ${VIRTUAL_TRANSPORT_LIBRARY}
  COMPONENT                 memchecker
  LIBRARY DESTINATION       lib
  ARCHIVE DESTINATION       lib
  RUNTIME DESTINATION       bin
  INCLUDES DESTINATION      ${VT_CONTEXT_EXTERNAL_DESTINATION}
)
