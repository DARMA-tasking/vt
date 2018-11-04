
set(TOP_LEVEL_SUBDIRS
  configs
  collective
  epoch
  group
  messaging
  pool
  pipe
  rdma
  topos
  vrt
  lb
  serialization
  utils
  registry
  runnable
  activefn
  # Add single-directory components
  context event handler parameterization sequence termination
  scheduler worker standalone runtime trace timing demangle
)
set(
  PROJECT_SUBDIRS_LIST
  # Add component along with sub-directories
    configs/debug configs/features configs/types configs/error
    collective/barrier collective/tree
    collective/reduce/
      collective/reduce/operators collective/reduce/functors
    collective/scatter/
    group/id group/region group/global group/msg group/collective group/rooted
    group/base
    pipe/id pipe/msg pipe/state pipe/signal pipe/interface pipe/callback
      pipe/callback/handler_send
      pipe/callback/handler_bcast
      pipe/callback/proxy_send
      pipe/callback/proxy_bcast
      pipe/callback/anon
      pipe/callback/cb_union
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
      vrt/collection/send vrt/collection/destroy vrt/collection/broadcast
      vrt/collection/insert vrt/collection/reducable vrt/collection/mapped_node
      vrt/collection/dispatch vrt/collection/gettable
      vrt/collection/balance
        vrt/collection/balance/hierarchicallb
        vrt/collection/balance/greedylb
        vrt/collection/balance/rotatelb
    lb/instrumentation
      lb/instrumentation/centralized
    lb/balancers
      lb/balancers/centralized
        lb/balancers/centralized/impls
    lb/migration
    serialization/messaging serialization/traits serialization/auto_dispatch
    utils/demangle utils/container utils/bits utils/mutex
    utils/hash utils/atomic utils/tls utils/static_checks utils/string
    registry/auto
      registry/auto/functor registry/auto/map registry/auto/collection
      registry/auto/vc registry/auto/rdma registry/auto/index

)
list(APPEND PROJECT_SUBDIRS_LIST ${TOP_LEVEL_SUBDIRS})

set(VT_INSTALL_DESTINATION "include/vt")
set(VT_EXTERNAL_DESTINATION "include")

foreach(DIR ${TOP_LEVEL_SUBDIRS})
  install(
    DIRECTORY                "vt/${DIR}"
    DESTINATION              ${VT_INSTALL_DESTINATION}
    CONFIGURATIONS           ${build_type_list}
    FILES_MATCHING PATTERN   "*.h"
  )
endforeach()

file(GLOB TOP_HEADERS "vt/*.h")

install(
  FILES                      ${TOP_HEADERS}
  DESTINATION                ${VT_INSTALL_DESTINATION}
  CONFIGURATIONS             ${build_type_list}
)

file(
  GLOB
  HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vt/*.h
)

file(
  GLOB
  SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vt/*.cc
)

foreach(SUB_DIR ${PROJECT_SUBDIRS_LIST})
  file(
    GLOB
    "${SUB_DIR}_HEADER_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/vt/${SUB_DIR}/*.h"
  )

  file(
    GLOB
    "${SUB_DIR}_SOURCE_FILES"
    RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/vt/${SUB_DIR}/*.cc"
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

target_compile_features(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC cxx_std_14
)

target_compile_features(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  cxx_variadic_templates
  cxx_auto_type
  cxx_constexpr
  cxx_decltype
  cxx_defaulted_functions
  cxx_deleted_functions
  cxx_defaulted_move_initializers
  cxx_delegating_constructors
  cxx_lambdas
  cxx_nonstatic_member_init
  cxx_nullptr
  cxx_override
  cxx_range_for
  cxx_right_angle_brackets
  cxx_uniform_initialization
  cxx_alias_templates
)

target_compile_options(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC ${CXX_STANDARD_FLAGS})
target_compile_options(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC ${VT_TARGET_CXX_FLAGS})

link_target_with_vt(
  TARGET ${VIRTUAL_TRANSPORT_LIBRARY}
  LINK_VT_LIB
  DEFAULT_LINK_SET
)

# link_target_with_vt(
#   TARGET ${VIRTUAL_TRANSPORT_LIBRARY_DV1}
#   BUILD_TYPE debug
#   LINK_VT_LIB
#   DEFAULT_LINK_SET
# )

# Define and set up the different build types of VT
include(../cmake/define_build_types.cmake)

target_include_directories(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:${VT_EXTERNAL_DESTINATION}>
)

install(
  TARGETS                   ${VIRTUAL_TRANSPORT_LIBRARY}
  EXPORT                    ${VIRTUAL_TRANSPORT_LIBRARY}
  CONFIGURATIONS            ${build_type_list}
  COMPONENT                 runtime
  LIBRARY DESTINATION       lib
  ARCHIVE DESTINATION       lib
  RUNTIME DESTINATION       bin
  INCLUDES DESTINATION      ${VT_EXTERNAL_DESTINATION}
)

install(
  EXPORT                    ${VIRTUAL_TRANSPORT_LIBRARY}
  DESTINATION               cmake
  NAMESPACE                 vt::runtime::
  FILE                      "vtTargets.cmake"
  CONFIGURATIONS            ${build_type_list}
  COMPONENT                 runtime
)

export(
  TARGETS                   ${VIRTUAL_TRANSPORT_LIBRARY} ${FCONTEXT_LIBRARY}
  FILE                      "vtTargets.cmake"
  NAMESPACE                 vt::runtime::
)
