function(create_trace_only_target)
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
    vt/configs/debug/debug_config.h
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
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../lib/fmt/include/fmt/core.h"
    DESTINATION "include/vt-trace/fmt"
  )

  set(VT_TRACE_LIB vt-trace CACHE INTERNAL "" FORCE)
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
endfunction(create_trace_only_target)

function(set_trace_only_config)
  # Set all variables for vt-trace-only mode
  # No need to worry about overriding the vt-runtime config file
  # since the scope is local to this function
  set(vt_feature_cmake_trace_enabled "1")
  set(vt_feature_cmake_trace_only "1")
  config_no_threading()

  configure_file(
    ${PROJECT_BASE_DIR}/cmake_config.h.in
    ${PROJECT_BIN_DIR}/${loop_build_type}/vt-trace/vt/cmake_config.h @ONLY
  )

  # In trace-only target we don't use INSTALL_DIR/include as include directory
  # We use INSTALL_DIR/include/vt-trace instead so we can use separate cmake_config.h
  install(
    FILES            "${PROJECT_BINARY_DIR}/${loop_build_type}/vt-trace/vt/cmake_config.h"
    DESTINATION      include/vt-trace/vt
    CONFIGURATIONS   ${loop_build_type}
  )

  set_target_properties(
    ${VT_TRACE_LIB}
    PROPERTIES ${loop_build_type_upper}_POSTFIX "-${loop_build_type}"
  )
endfunction(set_trace_only_config)