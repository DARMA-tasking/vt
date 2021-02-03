set(TOP_LEVEL_SUBDIRS
  configs
  collective
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
    collective
      collective/barrier
      collective/tree
      collective/scatter/
      collective/reduce/
        collective/reduce/operators collective/reduce/functors
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
        vrt/collection/balance/gossiplb
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
    utils/memory utils/mpi_limits
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
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES}
)

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

target_compile_options(${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC ${CXX_STANDARD_FLAGS} ${VT_TARGET_CXX_FLAGS})

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

# Define and set up the different build types of VT



######################
#  TRACE ONLY TARGET #
######################

if (vt_trace_only)

  # Don't GLOB all files, pick only the ones we're gonna use
  set(TRACE_HEADERS
    # vt/trace
    vt/trace/trace_common.h vt/trace/trace_constants.h
    vt/trace/trace_containers.h vt/trace/trace_event.h
    vt/trace/trace_log.h vt/trace/trace_user_event.h
    vt/trace/trace_user.h vt/trace/trace_lite.h

    # vt/runtime
    vt/runtime/mpi_access.h  vt/runtime/component/component_pack.h vt/runtime/component/component.h
    vt/runtime/component/component_registry.h vt/runtime/component/component_pack.impl.h
    vt/runtime/component/component_dep.h vt/runtime/component/component_traits.h
    vt/runtime/component/base.h vt/runtime/component/diagnostic.h
    vt/runtime/component/component_name.h vt/runtime/component/component_reduce.h
    vt/runtime/component/diagnostic_types.h vt/runtime/component/diagnostic_units.h
    vt/runtime/component/diagnostic_value.h vt/runtime/component/diagnostic_erased_value.h
    vt/runtime/component/diagnostic_value_base.h vt/runtime/component/diagnostic_meter.h
    vt/runtime/component/meter/counter.h vt/runtime/component/meter/stats_pack.h
    vt/runtime/component/meter/gauge.h vt/runtime/component/meter/timer.h
    vt/runtime/component/meter/counter_gauge.h
    vt/runtime/component/diagnostic.impl.h vt/runtime/component/bufferable.h
    vt/runtime/component/progressable.h vt/runtime/component/movable_fn.h

    # vt/timing
    vt/timing/timing.h vt/timing/timing_type.h

    # vt/context
    vt/context/context.h vt/context/context_attorney_fwd.h

    # vt/configs
    vt/configs/generated/vt_git_revision.h vt/configs/error/hard_error.h
    vt/configs/features/features_enableif.h vt/configs/features/features_featureswitch.h
    vt/configs/features/features_defines.h vt/configs/types/types_type.h
    vt/configs/error/pretty_print_message.h vt/configs/debug/debug_masterconfig.h
    vt/configs/features/features_metafeatures.h vt/configs/debug/debug_config.h
    vt/configs/debug/debug_printconst.h vt/configs/debug/debug_print.h
    vt/configs/arguments/app_config.h vt/configs/types/types_headers.h
    vt/configs/types/types_size.h vt/configs/types/types_rdma.h
    vt/configs/debug/debug_colorize.h vt/configs/debug/debug_var_unused.h
    vt/configs/error/error_headers.h vt/configs/error/soft_error.h
    vt/configs/error/common.h vt/configs/error/error.h vt/configs/error/error.impl.h
    vt/configs/error/config_assert.h vt/configs/error/assert_out.h
    vt/configs/error/assert_out.impl.h vt/configs/error/stack_out.h
    vt/configs/error/assert_out_info.h vt/configs/error/assert_out_info.impl.h
    vt/configs/error/keyval_printer.h vt/configs/error/keyval_printer.impl.h
    vt/configs/types/types_sentinels.h

    # vt/utils
    vt/utils/demangle/demangle.h vt/utils/bits/bits_counter.h
    vt/utils/bits/bits_common.h vt/utils/bits/bits_packer.h
    vt/utils/bits/bits_packer.impl.h vt/utils/adt/union.h
    vt/utils/tls/tls.h vt/utils/tls/std_tls.h vt/utils/tls/null_tls.h
    vt/utils/tls/tls.impl.h vt/utils/adt/histogram_approx.h

    # vt/collective
    vt/collective/basic.h

    # vt
    vt/config.h

    # vt/pmpi
    vt/pmpi/pmpi_component.h
  )

  set(TRACE_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/trace/trace_containers.cc ${CMAKE_CURRENT_SOURCE_DIR}/vt/trace/trace_event.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/trace/trace_lite.cc ${CMAKE_CURRENT_SOURCE_DIR}/vt/trace/trace_user_event.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/trace/trace_registry.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/pmpi_component.cc ${CMAKE_CURRENT_SOURCE_DIR}/vt/runtime/mpi_access.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/timing/timing.cc ${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc
    ${PROJECT_BIN_DIR}/src/vt/pmpi/generated/mpiwrap.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/context/context.cc ${CMAKE_CURRENT_SOURCE_DIR}/vt/utils/demangle/demangle.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/collective/basic.cc ${CMAKE_CURRENT_SOURCE_DIR}/vt/configs/error/pretty_print_message.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/vt/pmpi/pmpi_component.cc
  )

  set(TRACE_HEADER_FILES "")
  foreach ( FILE ${TRACE_HEADERS} )
    # Install in INSTALL_DIR/include/vt-trace/
    get_filename_component( DIR ${FILE} DIRECTORY )
    install(
      FILES ${FILE}
      DESTINATION "include/vt-trace/${DIR}"
    )

    # Populate TRACE_HEADER_FILES with absolute file paths to headers
    string(PREPEND ${FILE} ${CMAKE_CURRENT_SOURCE_DIR}/ NEW_FILE)
    list(APPEND TRACE_HEADER_FILES ${NEW_FILE})
  endforeach()

  # Since we're using separate cmake config file for vt-trace
  # we don't use INSTALL_DIR/include/ as include directory
  # we use INSTALL_DIR/include/vt-trace instead
  # so we have to install FMT lib aswell
  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../lib/fmt/fmt/format.h"
    DESTINATION "include/vt-trace/fmt"
  )

  set(VT_TRACE_LIB vt-trace)
  add_library(
    ${VT_TRACE_LIB}
    STATIC
    ${TRACE_HEADER_FILES} ${TRACE_SOURCE_FILES}
  )

  target_include_directories(
    ${VT_TRACE_LIB} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BASE_DIR}/src>
    $<INSTALL_INTERFACE:${VT_TRACE_EXTERNAL_DESTINATION}>
  )

  link_target_with_vt(
    TARGET ${VT_TRACE_LIB}
    LINK_VT_LIB
    LINK_FMT 1
    LINK_ZLIB 1
    LINK_MPI 1
  )

  set(VT_TRACE_TARGETS vt_trace_targets)

  install(
    TARGETS                   ${VT_TRACE_LIB}
    EXPORT                    ${VT_TRACE_TARGETS}
    CONFIGURATIONS            ${build_type_list}
    LIBRARY DESTINATION       lib
    ARCHIVE DESTINATION       lib
    RUNTIME DESTINATION       bin
    INCLUDES DESTINATION      ${VT_TRACE_EXTERNAL_DESTINATION}
  )

  install(
    EXPORT                    ${VT_TRACE_TARGETS}
    DESTINATION               cmake
    NAMESPACE                 vt::
    FILE                      "vt-traceTargets.cmake"
    CONFIGURATIONS            ${build_type_list}
  )

  export(
    TARGETS                   ${VT_TRACE_LIB}
    FILE                      "vt-traceTargets.cmake"
    NAMESPACE                 vt::
  )
endif()
######################
#   TRACE ONLY END   #
######################

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
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE --coverage)
  else()
    target_link_libraries(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE --coverage)
  endif()
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

if(CMAKE_VERSION VERSION_LESS "3.13.0")
  message(
    WARNING
    "VT: Install targets skipped because CMake version ${CMAKE_VERSION}<3.13; "
    "affects packages built with a VT install. "
    "CMake <3.13 does not support exporting targets from subdirectories"
  )
  set(cmake_do_install 0)
else()
  set(cmake_do_install 1)
endif()

if (cmake_do_install)
  if (vt_mimalloc_enabled)
    set(MIMALLOC_LIBRARY_EXPORT ${MIMALLOC_LIBRARY})
  endif()

  if (vt_libfort_enabled)
    set(FORT_LIBRARY_EXPORT ${FORT_LIBRARY})
    install(TARGETS ${FORT_LIBRARY} EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
  endif()

  if (vt_find_detector_dep EQUAL 0)
    install(TARGETS detector EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
    set(DETECTOR_EXPORT detector)
  endif()

  if (vt_find_checkpoint_dep EQUAL 0)
    install(TARGETS checkpoint EXPORT ${VIRTUAL_TRANSPORT_LIBRARY})
    set(CHECKPOINT_EXPORT checkpoint)
  endif()
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
                            ${CHEKPOINT_EXPORT}
                            ${DETECTOR_EXPORT}
  FILE                      "vtTargets.cmake"
  NAMESPACE                 vt::runtime::
)
