
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TOP_LEVEL_SUBDIRS
  configs
  collective
  group
  messaging
  pool
  rdma
  topos
  vrt
  lb
  serialization
  utils
  registry
  activefn
  # Add single-directory components
  context event handler parameterization sequence termination
  scheduler worker standalone runtime trace timing demangle
)
set(
  PROJECT_SUBDIRS_LIST
  # Add component along with sub-directories
    configs/debug configs/features configs/types
    collective/reduce collective/barrier collective/tree
    group/id group/region group/global
    messaging/envelope messaging/message
    pool/static_sized pool/header
    rdma/channel rdma/collection rdma/group rdma/state
    topos/location
      topos/location/cache topos/location/message topos/location/record
      topos/location/utility
    topos/index
      topos/index/dense topos/index/traits topos/index/printer
    topos/mapping
      topos/mapping/seed topos/mapping/dense
    vrt/base vrt/proxy vrt/context
    vrt/collection
      vrt/collection/types vrt/collection/messages vrt/collection/proxy_builder
      vrt/collection/holders vrt/collection/migrate vrt/collection/traits
      vrt/collection/traits vrt/collection/defaults vrt/collection/constructor
      vrt/collection/send
    lb/instrumentation
      lb/instrumentation/centralized
    lb/balancers
      lb/balancers/centralized
        lb/balancers/centralized/impls
    lb/migration
    serialization/messaging serialization/traits
    utils/demangle utils/container utils/bits utils/mutex
    utils/hash utils/atomic utils/tls utils/static_checks
    registry/auto
      registry/auto/functor registry/auto/map registry/auto/collection
      registry/auto/vc

)
list(APPEND PROJECT_SUBDIRS_LIST ${TOP_LEVEL_SUBDIRS})

foreach(DIR ${TOP_LEVEL_SUBDIRS})
install(DIRECTORY ${DIR} DESTINATION include
        FILES_MATCHING PATTERN "*.h")
endforeach()

file(GLOB TOP_HEADERS *.h)
install(FILES ${TOP_HEADERS} DESTINATION include)



include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib/context/src)
include_directories(${PROJECT_LIB_DIR})
include_directories(${DARMA_DETECTOR_PATH}/include)
include_directories(${DARMA_MELD_PATH}/include)
include_directories(${CMAKE_BINARY_DIR})

file(
  GLOB
  HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

file(
  GLOB
  SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

foreach(SUB_DIR ${PROJECT_SUBDIRS_LIST})
  file(
    GLOB
    "${SUB_DIR}_HEADER_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.h"
  )

  file(
    GLOB
    "${SUB_DIR}_SOURCE_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.cc"
  )

  list(
    APPEND
    HEADER_FILES
    "${${SUB_DIR}_HEADER_FILES}"
  )

  list(
    APPEND
    SOURCE_FILES
    "${${SUB_DIR}_SOURCE_FILES}"
  )
endforeach()

add_library(
  ${VIRTUAL_TRANSPORT_LIBRARY}
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES}
)
target_link_libraries(${VIRTUAL_TRANSPORT_LIBRARY} ${ZLIB_LIBRARIES} darma_meld darma_detector fmt::fmt)

install(
  TARGETS ${VIRTUAL_TRANSPORT_LIBRARY}
  EXPORT ${VIRTUAL_TRANSPORT_LIBRARY}
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib"
)

install(
  EXPORT vt
  DESTINATION cmake 
  COMPONENT extCfg
)

