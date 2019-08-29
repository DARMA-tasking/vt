# need a lower-case subdirectory name to use for the chosen build type
if (NOT CMAKE_BUILD_TYPE)
  message(WARNING "No CMAKE_BUILD_TYPE detected")
  set(lower_CMAKE_BUILD_TYPE "undefined")
else()
  string(TOLOWER ${CMAKE_BUILD_TYPE} lower_CMAKE_BUILD_TYPE)
endif()

# enumerate configurations we will support
if (NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(VT_CONFIG_TYPES ${lower_CMAKE_BUILD_TYPE} "debug" "release" "relwithdebinfo")
else()
  set(VT_CONFIG_TYPES ${lower_CMAKE_BUILD_TYPE})
  foreach(type ${CMAKE_CONFIGURATION_TYPES})
    string(TOLOWER ${type} tmp_lowercase_type)
    list(APPEND VT_CONFIG_TYPES ${tmp_lowercase_type})
  endforeach(type)
  unset(tmp_lowercase_type)
endif()
list(REMOVE_DUPLICATES VT_CONFIG_TYPES)

# to speedup compiling, we can disable debug_print
if (${VT_DEBUG_FAST})
  set(VT_DEBUG_MODE_ON 0)
else()
  set(VT_DEBUG_MODE_ON 1)
endif()

# all debug_print and vt_print categories we can potentially compile in
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
set(cmake_vt_debug_modes_relwithdebinfo        "")
set(cmake_vt_debug_modes_release               "")
set(cmake_config_debug_enabled_debug           ${VT_DEBUG_MODE_ON})
set(cmake_config_debug_enabled_relwithdebinfo  0)
set(cmake_config_debug_enabled_release         0)

# this loop executes over all known build types, not just the selected one
foreach(loop_build_type ${VT_CONFIG_TYPES})
  #message(STATUS "generating for build type=${loop_build_type}")

  # disable debug_print for unfamiliar build types
  if (NOT DEFINED cmake_vt_debug_modes_${loop_build_type})
    set(cmake_vt_debug_modes_${loop_build_type} "")
  endif()
  if (NOT DEFINED cmake_config_debug_enabled_${loop_build_type})
    set(cmake_config_debug_enabled_${loop_build_type} 0)
  endif()

  # use the debug_print modes specified for this build type before the loop
  set(
    cmake_vt_debug_modes
    ${cmake_vt_debug_modes_${loop_build_type}}
  )

  # assume production mode for everything except debug
  if (loop_build_type STREQUAL "debug")
    set(vt_feature_cmake_production "0")
  else()
    set(vt_feature_cmake_production "1")
  endif()

  # use the debug_print configuration specified for this build type before the loop
  set(
    cmake_config_debug_enabled
    ${cmake_config_debug_enabled_${loop_build_type}}
  )

  # put the config file in a subdirectory corresponding to the lower case build name
  configure_file(
    ${PROJECT_BASE_DIR}/cmake_config.h.in
    ${PROJECT_BIN_DIR}/${loop_build_type}/cmake_config.h @ONLY
  )

  # install the correct config file when this build is selected
  install(
    FILES            "${PROJECT_BINARY_DIR}/${loop_build_type}/cmake_config.h"
    DESTINATION      include
    CONFIGURATIONS   ${loop_build_type}
  )

  # append the lower-case build type to the library name
  string(TOUPPER ${loop_build_type} loop_build_type_upper)
  set(CMAKE_${loop_build_type_upper}_POSTFIX "-${loop_build_type}")
  set_target_properties(
    ${VIRTUAL_TRANSPORT_LIBRARY}
    PROPERTIES ${loop_build_type_upper}_POSTFIX "-${loop_build_type}"
  )
endforeach()

# message(STATUS "chosen build type=${CMAKE_BUILD_TYPE}")

# build types below are not case sensitive (but the directories are)
target_include_directories(
  ${VIRTUAL_TRANSPORT_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:$<$<CONFIG:debug>:${PROJECT_BIN_DIR}/debug>>
  $<BUILD_INTERFACE:$<$<CONFIG:relwithdebinfo>:${PROJECT_BIN_DIR}/relwithdebinfo>>
  $<BUILD_INTERFACE:$<$<CONFIG:release>:${PROJECT_BIN_DIR}/release>>
  $<BUILD_INTERFACE:$<$<CONFIG:${CMAKE_BUILD_TYPE}>:${PROJECT_BIN_DIR}/${lower_CMAKE_BUILD_TYPE}>>
  $<BUILD_INTERFACE:$<$<CONFIG:>:${PROJECT_BIN_DIR}/undefined>>
  $<INSTALL_INTERFACE:include>
)

