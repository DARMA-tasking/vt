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

# to speedup compiling, we can disable vt_debug_print
if (${VT_DEBUG_FAST})
  set(VT_DEBUG_MODE_ON 0)
else()
  set(VT_DEBUG_MODE_ON 1)
endif()

# all vt_debug_print and vt_print categories we can potentially compile in
set(
  cmake_vt_debug_modes_all
  "CatEnum::gen          | \
   CatEnum::runtime      | \
   CatEnum::active       | \
   CatEnum::term         | \
   CatEnum::termds       | \
   CatEnum::barrier      | \
   CatEnum::pipe         | \
   CatEnum::param        | \
   CatEnum::pool         | \
   CatEnum::reduce       | \
   CatEnum::rdma         | \
   CatEnum::rdma_channel | \
   CatEnum::handler      | \
   CatEnum::hierlb       | \
   CatEnum::gossiplb     | \
   CatEnum::scatter      | \
   CatEnum::serial_msg   | \
   CatEnum::sequence     | \
   CatEnum::trace        | \
   CatEnum::objgroup     | \
   CatEnum::location     | \
   CatEnum::lb           | \
   CatEnum::vrt_coll     | \
   CatEnum::group        | \
   CatEnum::phase        | \
   CatEnum::epoch        | \
   CatEnum::broadcast      \
   "
)

option(vt_detector_disabled "Build VT with detector disabled" OFF)
option(vt_lb_enabled "Build VT with load balancing enabled" ON)
option(vt_trace_enabled "Build VT with trace enabled" OFF)
option(vt_priorities_enabled "Build VT with message priorities enabled" ON)
option(
  vt_test_trace_runtime_enabled
  "Build VT with runtime tracing enabled (for testing)" OFF
)
option(vt_pool_enabled "Build VT with memory pool" ON)

set(
  vt_priority_bits_per_level 3 CACHE
  STRING "Number of bits to use per VT priority level"
)

if (${vt_test_trace_runtime_enabled})
  message(STATUS "Building VT with runtime tracing enabled (for testing)")
  set(vt_feature_cmake_test_trace_on "1")
else()
  set(vt_feature_cmake_test_trace_on "0")
endif()

if (${vt_detector_disabled})
  message(STATUS "Building VT with detector disabled")
  set(vt_feature_cmake_detector "0")
else()
  message(STATUS "Building VT with detector enabled")
  set(vt_feature_cmake_detector "1")
endif()

if (${vt_lb_enabled})
  message(STATUS "Building VT with load balancing enabled")
  set(vt_feature_cmake_lblite "1")
else()
  message(STATUS "Building VT with load balancing disabled")
  set(vt_feature_cmake_lblite "0")
endif()

if (${vt_trace_enabled})
  message(STATUS "Building VT with tracing enabled")
  set(vt_feature_cmake_trace_enabled "1")
else()
  message(STATUS "Building VT with tracing disabled")
  set(vt_feature_cmake_trace_enabled "0")
endif()

# This will be set to 1 during generation of cmake config for vt-trace target
set(vt_feature_cmake_trace_only "0")

if (${vt_priorities_enabled})
  message(STATUS "Building VT with priorities enabled")
  message(
    STATUS
    "Building VT with priority bits per level: ${vt_priority_bits_per_level}"
  )
  set(vt_feature_cmake_priorities "1")
else()
  message(STATUS "Building VT with priorities disabled")
  set(vt_feature_cmake_priorities "0")
endif()

set(vt_feature_cmake_priority_bits_level "${vt_priority_bits_per_level}")

if (${vt_bit_check_overflow})
  message(STATUS "Building VT with bit check overflow")
  set(vt_feature_cmake_bit_check_overflow "1")
else()
  set(vt_feature_cmake_bit_check_overflow "0")
endif()

if (${vt_fcontext_enabled})
  message(STATUS "Building VT with fcontext (ULT) enabled")
  set(vt_feature_cmake_fcontext "1")
else()
  message(STATUS "Building VT with fcontext (ULT) disabled")
  set(vt_feature_cmake_fcontext "0")
endif()

if (${vt_mimalloc_enabled})
  message(STATUS "Building VT with mimalloc enabled")
  set(vt_feature_cmake_mimalloc "1")
else()
  message(STATUS "Building VT with mimalloc disabled")
  set(vt_feature_cmake_mimalloc "0")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_CMAKE_BUILD_TYPE)
if (LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "debug")
  option(vt_mpi_guards "Build VT with poison MPI calls: code invoked from VT callbacks cannot invoke MPI functions" ON)
else()
  option(vt_mpi_guards "Build VT with poison MPI calls: code invoked from VT callbacks cannot invoke MPI functions" OFF)
endif()

if ((vt_mpi_guards OR vt_trace_only) AND PERL_FOUND)
  message(STATUS "Building VT with user MPI prevention guards enabled")
  set(vt_feature_cmake_mpi_access_guards "1")
elseif ((vt_mpi_guards OR vt_trace_only) AND NOT PERL_FOUND)
  # No perl? Can't generate wrapper source file.
  message(STATUS "Building VT with user MPI prevention guards disabled (requested, but perl not found)")
  set(vt_feature_cmake_mpi_access_guards "0")
else()
  message(STATUS "Building VT with user MPI prevention guards disabled")
  set(vt_feature_cmake_mpi_access_guards "0")
endif()

option(vt_zoltan_enabled "Build VT with Zoltan" OFF)
if (vt_zoltan_enabled AND vt_zoltan_found)
  message(STATUS "Building VT with zoltan enabled")
  set(vt_feature_cmake_zoltan "1")
elseif (vt_zoltan_enabled AND NOT vt_zoltan_found)
  message(STATUS "Building VT with zoltan disabled (requested, but Zoltan not found)")
  set(vt_feature_cmake_zoltan "0")
else()
  message(STATUS "Building VT with zoltan disabled")
  set(vt_feature_cmake_zoltan "0")
endif()

option(vt_ci_build "Build VT with CI mode on" OFF)
if(${vt_ci_build})
  set(vt_feature_cmake_ci_build "1")
else()
  set(vt_feature_cmake_ci_build "0")
endif()

message(STATUS "CI_BUILD = ${vt_feature_cmake_ci_build}")

option(
  vt_diagnostics_runtime_enabled
  "Build VT with performance metrics/stats enabled at runtime by default" OFF
)
if (vt_diagnostics_enabled)
  message(STATUS "Building VT with diagnostics (performance stats) enabled")
  set(vt_feature_cmake_diagnostics "1")

  if (vt_diagnostics_runtime_enabled)
    message(
      STATUS
      "Building VT with diagnostics (performance stats) enabled at runtime by default"
    )
    set(vt_feature_cmake_diagnostics_runtime "1")
  else()
    message(
      STATUS
      "Building VT with diagnostics (performance stats) disabled at runtime by default"
    )
    set(vt_feature_cmake_diagnostics_runtime "0")
  endif()

else()
  message(STATUS "Building VT with diagnostics (performance stats) disabled")
  set(vt_feature_cmake_diagnostics "0")
  set(vt_feature_cmake_diagnostics_runtime "0")
endif()

if (vt_libfort_enabled)
  set(vt_feature_cmake_libfort "1")
else()
  set(vt_feature_cmake_libfort "0")
endif()

set(vt_feature_cmake_no_feature "0")
set(vt_feature_cmake_production "0")

set (vt_feature_cmake_mpi_rdma "0")
set (vt_feature_cmake_print_term_msgs "0")
set (vt_feature_cmake_no_pool_alloc_env "0")

if (${vt_pool_enabled})
  message(STATUS "Building VT with memory pool enabled")
  set (vt_feature_cmake_memory_pool "1")
else()
  message(STATUS "Building VT with memory pool disabled")
  set (vt_feature_cmake_memory_pool "0")
endif()

set (vt_feature_cmake_cons_multi_idx "0")

set(cmake_vt_debug_modes_debug                 "${cmake_vt_debug_modes_all}")
set(cmake_vt_debug_modes_relwithdebinfo        "")
set(cmake_vt_debug_modes_release               "")
set(cmake_config_debug_enabled_debug           ${VT_DEBUG_MODE_ON})
set(cmake_config_debug_enabled_relwithdebinfo  0)
set(cmake_config_debug_enabled_release         0)

set(
  cmake_vt_modes
  ${cmake_vt_debug_modes_all}
)

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

  # assume production mode for everything except debug or CI build
  if (loop_build_type STREQUAL "debug" OR ${vt_feature_cmake_ci_build})
    set(vt_feature_cmake_production "0")
  else()
    set(vt_feature_cmake_production "1")
  endif()

  # use the vt_debug_print configuration specified for this build type before
  # the loop
  set(
    cmake_config_debug_enabled
    ${cmake_config_debug_enabled_${loop_build_type}}
  )

  # put the config file in a subdirectory corresponding to the lower case build name
  configure_file(
    ${PROJECT_BASE_DIR}/cmake_config.h.in
    ${PROJECT_BIN_DIR}/${loop_build_type}/vt/cmake_config.h @ONLY
  )

  # install the correct config file when this build is selected
  install(
    FILES            "${PROJECT_BINARY_DIR}/${loop_build_type}/vt/cmake_config.h"
    DESTINATION      include/vt
    CONFIGURATIONS   ${loop_build_type}
  )

  # append the lower-case build type to the library name
  string(TOUPPER ${loop_build_type} loop_build_type_upper)
  set(CMAKE_${loop_build_type_upper}_POSTFIX "-${loop_build_type}")
  set_target_properties(
    ${VIRTUAL_TRANSPORT_LIBRARY}
    PROPERTIES ${loop_build_type_upper}_POSTFIX "-${loop_build_type}"
  )

  # Generate separate config file for vt-trace
  if (vt_trace_only)
    set_trace_only_config()
  endif()
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

if (vt_trace_only)
  target_include_directories(
    ${VT_TRACE_LIB} PUBLIC
    $<BUILD_INTERFACE:$<$<CONFIG:debug>:${PROJECT_BIN_DIR}/debug/vt-trace>>
    $<BUILD_INTERFACE:$<$<CONFIG:relwithdebinfo>:${PROJECT_BIN_DIR}/relwithdebinfo/vt-trace>>
    $<BUILD_INTERFACE:$<$<CONFIG:release>:${PROJECT_BIN_DIR}/release/vt-trace>>
    $<BUILD_INTERFACE:$<$<CONFIG:${CMAKE_BUILD_TYPE}>:${PROJECT_BIN_DIR}/${lower_CMAKE_BUILD_TYPE}/vt-trace>>
    $<BUILD_INTERFACE:$<$<CONFIG:>:${PROJECT_BIN_DIR}/undefined>>
    $<INSTALL_INTERFACE:include/vt-trace>
  )
endif()
