


# this loop executes over all known build types, not just the selected one
foreach(loop_build_type ${VT_CONFIG_TYPES})
  #message(STATUS "generating for build type=${loop_build_type}")

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
