set(TOP_LEVEL_SUBDIRS
  configs
  collective
  elm
  epoch
  group
  messaging
  objgroup
  phase
  pool
  pipe
  pmpi
  rdma
  topos
  vrt
  serialization
  utils
  registry
  runnable
  activefn
  # Add single-directory components
  context event handler parameterization sequence termination
  scheduler worker standalone runtime trace timing demangle rdmahandle
)
set(
  PROJECT_SUBDIRS_LIST
  # Add component along with sub-directories
    configs/debug configs/features configs/types configs/error configs/arguments
    configs/generated
    context/runnable_context
    collective
      collective/barrier
      collective/tree
      collective/scatter/
      collective/reduce/
        collective/reduce/operators collective/reduce/functors
    elm
    group/id group/region group/global group/msg group/collective group/rooted
    group/base
    pipe/id pipe/msg pipe/state pipe/signal pipe/interface pipe/callback
      pipe/callback/handler_send
      pipe/callback/handler_bcast
      pipe/callback/proxy_send
      pipe/callback/proxy_bcast
      pipe/callback/objgroup_send
      pipe/callback/objgroup_bcast
      pipe/callback/anon
      pipe/callback/cb_union
    timetrigger
    termination
      termination/dijkstra-scholten
      termination/interval
      termination/graph
    messaging/envelope messaging/message
    phase
    pool/static_sized pool/header
    rdma/channel rdma/collection rdma/group rdma/state
    rdmahandle
    topos/location
      topos/location/cache topos/location/message topos/location/record
      topos/location/utility
      topos/location/directory
      topos/location/lookup
    topos/index
      topos/index/dense topos/index/traits topos/index/printer
    topos/mapping
      topos/mapping/seed topos/mapping/dense
    runtime
      runtime/component
      runtime/component/meter
    vrt/base vrt/proxy vrt/context
    vrt/collection
      vrt/collection/types vrt/collection/messages vrt/collection/proxy_builder
      vrt/collection/holders vrt/collection/migrate vrt/collection/traits
      vrt/collection/traits vrt/collection/defaults vrt/collection/constructor
      vrt/collection/send vrt/collection/destroy vrt/collection/broadcast
      vrt/collection/insert vrt/collection/reducable vrt/collection/mapped_node
      vrt/collection/dispatch vrt/collection/gettable vrt/collection/rdmaable
      vrt/collection/staged_token
      vrt/collection/listener
      vrt/collection/invoke
      vrt/collection/types/storage
      vrt/collection/balance
        vrt/collection/balance/baselb
        vrt/collection/balance/hierarchicallb
        vrt/collection/balance/greedylb
        vrt/collection/balance/rotatelb
        vrt/collection/balance/temperedlb
        vrt/collection/balance/statsmaplb
        vrt/collection/balance/zoltanlb
        vrt/collection/balance/randomlb
        vrt/collection/balance/lb_invoke
        vrt/collection/balance/model
        vrt/collection/balance/proxy
    serialization
      serialization/messaging serialization/traits serialization/auto_dispatch
      serialization/sizing
    utils/demangle utils/container utils/bits utils/mutex
    utils/hash utils/atomic utils/tls utils/static_checks utils/string
    utils/memory utils/mpi_limits utils/compress utils/json utils/strong
    registry/auto
      registry/auto/functor registry/auto/map registry/auto/collection
      registry/auto/vc registry/auto/rdma registry/auto/index
    objgroup
      objgroup/proxy objgroup/holder objgroup/active_func objgroup/dispatch
      objgroup/type_registry
    trace
      trace/file_spec
)
list(APPEND PROJECT_SUBDIRS_LIST ${TOP_LEVEL_SUBDIRS})

set(VT_INSTALL_DESTINATION "include/vt")
set(VT_TRACE_INSTALL_DESTINATION "include/vt-trace")

set(VT_EXTERNAL_DESTINATION "include")
set(VT_TRACE_EXTERNAL_DESTINATION "include/vt-trace")

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
file(
  GLOB
  SOURCE_FILES
  ${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc
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


include(../cmake/configure_options.cmake)

# Generate PMPI wrappers, if enabled and possible.
if((vt_mpi_guards OR vt_trace_only) AND PERL_FOUND)
  set(MPI_WRAP_GENERATED "${PROJECT_BIN_DIR}/src/vt/pmpi/generated/mpiwrap.cc")

  file(MAKE_DIRECTORY "${PROJECT_BIN_DIR}/src/vt/pmpi/generated")

  add_custom_command(
    OUTPUT ${MPI_WRAP_GENERATED}
    COMMAND ${PERL_EXECUTABLE}
    ARGS "${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/generate_mpi_wrappers.pl"
         "${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/mpi_functions.h.in"
         "${MPI_WRAP_GENERATED}"
    DEPENDS
         "${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/generate_mpi_wrappers.pl"
         "${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/mpi_functions.h.in"
  )

  list(
    APPEND
    SOURCE_FILES
    ${MPI_WRAP_GENERATED}
  )
endif()

add_library(
  ${VIRTUAL_TRANSPORT_LIBRARY}
  ${HEADER_FILES} ${SOURCE_FILES}
)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/build_git_info.cmake)

option(vt_no_color_enabled "Build VT with option --vt_no_color set to true by default" OFF)
if(vt_no_color_enabled)
  message(STATUS "Building VT with --vt_no_color set to true by default")
  target_compile_definitions(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC VT_NO_COLOR_ENABLED)
else()
  message(STATUS "Building VT with --vt_no_color set to false by default")
endif()

target_compile_features(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  cxx_std_14
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

include(turn_on_warnings)
turn_on_warnings(${VIRTUAL_TRANSPORT_LIBRARY})

include(SetCXXCompilerFlags)
set_darma_compiler_flags(${VIRTUAL_TRANSPORT_LIBRARY})

if (vt_gold_linker_enabled)
  if (UNIX AND NOT APPLE)
    execute_process(
      COMMAND
        ${CMAKE_C_COMPILER}
        -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE ld_version
    )
    if ("${ld_version}" MATCHES "GNU gold")
      set(
        CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags"
      )
      set(
        CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags"
      )
    endif()
  endif()
endif()

if (vt_unity_build_enabled)
  set_target_properties(${VIRTUAL_TRANSPORT_LIBRARY} PROPERTIES UNITY_BUILD ON)
endif()

link_target_with_vt(
  TARGET ${VIRTUAL_TRANSPORT_LIBRARY}
  LINK_VT_LIB
  DEFAULT_LINK_SET
)

include(../cmake/trace_only_functions.cmake)
if (vt_trace_only)
  create_trace_only_target()
endif()

# Define and set up the different build types of VT
include(../cmake/define_build_types.cmake)

target_include_directories(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_BASE_DIR}/src>
  $<INSTALL_INTERFACE:${VT_EXTERNAL_DESTINATION}>
)

if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "VT: enabling code coverage generation")
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
    -Og
    --coverage # sets all required flags
  )
  target_link_options(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE --coverage)
endif()

include(../cmake/set_linker_flags.cmake)

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

set(FMT_LIBRARY_EXPORT ${FMT_LIBRARY})
install(TARGETS ${FMT_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
install(TARGETS ${ENG_FORMAT_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})

install(TARGETS ${JSON_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
install(TARGETS ${BROTLI_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})

if (vt_mimalloc_enabled)
  set(MIMALLOC_LIBRARY_EXPORT ${MIMALLOC_LIBRARY})
endif()

if (vt_libfort_enabled)
  set(FORT_LIBRARY_EXPORT ${FORT_LIBRARY})
  install(TARGETS ${FORT_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
endif()

if (vt_fcontext_enabled)
  set(FCONTEXT_LIBRARY_EXPORT ${FCONTEXT_LIBRARY})
endif()

# Export to build directory
export(
  TARGETS                   ${VIRTUAL_TRANSPORT_LIBRARY}
                            ${FCONTEXT_LIBRARY_EXPORT}
                            ${MIMALLOC_LIBRARY_EXPORT}
                            ${FORT_LIBRARY_EXPORT}
                            ${FMT_LIBRARY_EXPORT}
                            ${ENG_FORMAT_LIBRARY}
                            ${JSON_LIBRARY}
                            ${BROTLI_LIBRARY}
  FILE                      "vtTargets.cmake"
  NAMESPACE                 vt::runtime::
)
