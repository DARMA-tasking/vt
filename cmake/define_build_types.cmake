
set(CMAKE_CONFIGURATION_TYPES "debug;release" CACHE STRING "" FORCE)

if (${VT_DEBUG_FAST})
  set(VT_DEBUG_MODE_ON 0)
else()
  set(VT_DEBUG_MODE_ON 1)
endif()

set(
  cmake_vt_debug_modes_all
  "CatEnum::gen          | \
   CatEnum::runtime      | \
   CatEnum::active       | \
   CatEnum::term         | \
   CatEnum::termds       | \
   CatEnum::barrier      | \
   CatEnum::pipe         | \
   CatEnum:: pool        | \
   CatEnum::reduce       | \
   CatEnum::rdma         | \
   CatEnum::rdma_channel | \
   CatEnum::handler      | \
   CatEnum::hierlb       | \
   CatEnum::scatter      | \
   CatEnum::serial_msg   | \
   CatEnum::trace        | \
   CatEnum::objgroup     | \
   CatEnum::location     | \
   CatEnum::lb           | \
   CatEnum::vrt_coll     | \
   CatEnum::group        | \
   CatEnum::broadcast      \
   "
)

if (${vt_detector_disabled})
  message(STATUS "Building VT with detector disabled")
  set(vt_feature_cmake_detector "0")
else()
  set(vt_feature_cmake_detector "1")
endif()

if (${vt_lb_enabled})
  message(STATUS "Building VT with load balancing enabled")
  set(vt_feature_cmake_lblite "1")
else()
  set(vt_feature_cmake_lblite "0")
endif()

if (${vt_trace_enabled})
  message(STATUS "Building VT with tracing enabled")
  set(vt_feature_cmake_trace_enabled "1")
else()
  set(vt_feature_cmake_trace_enabled "0")
endif()

if (${vt_bit_check_overflow})
  message(STATUS "Building VT with bit check overflow")
  set(vt_feature_cmake_bit_check_overflow "1")
else()
  set(vt_feature_cmake_bit_check_overflow "0")
endif()

set(vt_feature_cmake_no_feature "0")
set(vt_feature_cmake_production "0")

set (vt_feature_cmake_mpi_rdma "0")
set (vt_feature_cmake_parserdes "0")
set (vt_feature_cmake_print_term_msgs "0")
set (vt_feature_cmake_default_threading "1")
set (vt_feature_cmake_no_pool_alloc_env "0")
set (vt_feature_cmake_memory_pool "1")
set (vt_feature_cmake_cons_multi_idx "0")

set(cmake_vt_debug_modes_debug                 "${cmake_vt_debug_modes_all}")
set(cmake_vt_debug_modes_release               "")
set(cmake_config_debug_enabled_debug           ${VT_DEBUG_MODE_ON})
set(cmake_config_debug_enabled_release         0)

set(build_type_list)

foreach(cur_build_type ${CMAKE_CONFIGURATION_TYPES})
  #message(STATUS "generating for build type=${cur_build_type}")

  set(
    cmake_vt_debug_modes
    ${cmake_vt_debug_modes_${cur_build_type}}
  )

  if (cur_build_type STREQUAL "release")
    set(vt_feature_cmake_production "1")
  endif()

  set(
    cmake_config_debug_enabled
    ${cmake_config_debug_enabled_${cur_build_type}}
  )

  configure_file(
    ${PROJECT_BASE_DIR}/cmake_config.h.in
    ${PROJECT_BIN_DIR}/${cur_build_type}/cmake_config.h @ONLY
  )

  install(
    FILES            "${PROJECT_BINARY_DIR}/${cur_build_type}/cmake_config.h"
    DESTINATION      include
    CONFIGURATIONS   ${cur_build_type}
  )

  string(TOUPPER ${cur_build_type} cur_build_type_upper)

  set(CMAKE_${cur_build_type_upper}_POSTFIX "-${cur_build_type}")

  list(APPEND build_type_list ${cur_build_type})

  set_target_properties(
    ${VIRTUAL_TRANSPORT_LIBRARY}
    PROPERTIES ${cur_build_type_upper}_POSTFIX "-${cur_build_type}"
  )
endforeach()

# message(STATUS "build type list=${build_type_list}")
# message(STATUS "build type=${CMAKE_BUILD_TYPE}")

target_include_directories(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:$<$<CONFIG:debug>:${PROJECT_BIN_DIR}/debug>>
  $<BUILD_INTERFACE:$<$<CONFIG:release>:${PROJECT_BIN_DIR}/release>>
  $<INSTALL_INTERFACE:include>
)

