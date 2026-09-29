include_guard(GLOBAL)

# Configure one compiled fmt provider across a hierarchy of DARMA projects.
#
# A top-level project builds its vendored fmt. An embedded project reuses the
# parent's fmt::fmt target and only contributes its local include layout (for
# example fmt-vt, fmt, or fmt-lb). Exact FMT_VERSION equality is required so
# local headers never call into an incompatible compiled fmt implementation.
#
# Copy this file into each repository and replace add_subdirectory(fmt) with:
#
#   include(cmake/setup_fmt.cmake)
#   darma_setup_fmt(
#     SOURCE_DIR         lib/fmt
#     VENDORED_TARGET    fmt
#     OUT_LIBRARY        FMT_LIBRARY
#     OUT_LINK_TARGET    FMT_LINK_TARGET
#   )
#
# VT, comm, and loc use lib/fmt + fmt; LB uses tpl/fmt + fmt-lb.
# Optional POST_ADD_COMMAND names a project function which is called only when
# the current project owns and compiles the vendored target.
function(darma_setup_fmt)
  cmake_parse_arguments(
    PARSE_ARGV 0
    DARMA_FMT
    ""
    "SOURCE_DIR;VENDORED_TARGET;OUT_LIBRARY;OUT_LINK_TARGET;POST_ADD_COMMAND"
    ""
  )

  foreach(required_arg
      SOURCE_DIR VENDORED_TARGET OUT_LIBRARY OUT_LINK_TARGET)
    if(NOT DARMA_FMT_${required_arg})
      message(FATAL_ERROR "darma_setup_fmt requires ${required_arg}")
    endif()
  endforeach()

  if(IS_ABSOLUTE "${DARMA_FMT_SOURCE_DIR}")
    set(fmt_source_dir "${DARMA_FMT_SOURCE_DIR}")
  else()
    get_filename_component(
      fmt_source_dir
      "${DARMA_FMT_SOURCE_DIR}"
      ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()

  file(GLOB fmt_base_headers "${fmt_source_dir}/include/*/base.h")
  list(LENGTH fmt_base_headers fmt_base_header_count)
  if(NOT fmt_base_header_count EQUAL 1)
    message(
      FATAL_ERROR
      "Expected one fmt base.h below ${fmt_source_dir}/include, found "
      "${fmt_base_header_count}"
    )
  endif()

  list(GET fmt_base_headers 0 fmt_base_header)
  file(STRINGS
    "${fmt_base_header}"
    fmt_version_line
    REGEX "^# *define FMT_VERSION [0-9]+"
    LIMIT_COUNT 1
  )
  if(NOT fmt_version_line MATCHES "FMT_VERSION +([0-9]+)")
    message(FATAL_ERROR "Could not read FMT_VERSION from ${fmt_base_header}")
  endif()
  set(local_fmt_version "${CMAKE_MATCH_1}")

  # Use the source-directory comparison rather than PROJECT_IS_TOP_LEVEL so
  # the same module also works with LB's current CMake 3.20 minimum.
  if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    if(TARGET fmt::fmt)
      message(
        FATAL_ERROR
        "${PROJECT_NAME} is top-level, but fmt::fmt already exists. The "
        "top-level DARMA project must be the sole compiled fmt provider."
      )
    endif()

    add_subdirectory("${fmt_source_dir}")

    if(NOT TARGET ${DARMA_FMT_VENDORED_TARGET})
      message(
        FATAL_ERROR
        "${fmt_source_dir} did not create target "
        "'${DARMA_FMT_VENDORED_TARGET}'"
      )
    endif()
    # VT and comm already expose fmt::fmt. LB's renamed vendored target is
    # fmt-lb, so supply the common provider alias here when it is absent.
    if(NOT TARGET fmt::fmt)
      add_library(fmt::fmt ALIAS ${DARMA_FMT_VENDORED_TARGET})
    endif()

    if(DARMA_FMT_POST_ADD_COMMAND)
      if(NOT COMMAND ${DARMA_FMT_POST_ADD_COMMAND})
        message(
          FATAL_ERROR
          "Unknown POST_ADD_COMMAND: ${DARMA_FMT_POST_ADD_COMMAND}"
        )
      endif()
      cmake_language(
        CALL ${DARMA_FMT_POST_ADD_COMMAND} ${DARMA_FMT_VENDORED_TARGET}
      )
    endif()

    set_property(
      TARGET ${DARMA_FMT_VENDORED_TARGET}
      PROPERTY INTERFACE_DARMA_FMT_VERSION "${local_fmt_version}"
    )
    set(fmt_owned_library "${DARMA_FMT_VENDORED_TARGET}")
    message(
      STATUS
      "${PROJECT_NAME}: building vendored fmt ${local_fmt_version}"
    )
  else()
    if(NOT TARGET fmt::fmt)
      message(
        FATAL_ERROR
        "${PROJECT_NAME} is embedded, but its parent did not provide "
        "fmt::fmt. Configure fmt before add_subdirectory(${PROJECT_NAME})."
      )
    endif()

    get_target_property(fmt_provider_target fmt::fmt ALIASED_TARGET)
    if(NOT fmt_provider_target)
      set(fmt_provider_target fmt::fmt)
    endif()
    get_target_property(
      parent_fmt_version
      ${fmt_provider_target}
      INTERFACE_DARMA_FMT_VERSION
    )
    if(NOT parent_fmt_version)
      message(
        FATAL_ERROR
        "The parent fmt::fmt target was not configured by setup_fmt.cmake; "
        "its version cannot be verified."
      )
    endif()
    if(NOT local_fmt_version STREQUAL parent_fmt_version)
      message(
        FATAL_ERROR
        "fmt version mismatch: ${PROJECT_NAME} has ${local_fmt_version}, "
        "but the parent provider has ${parent_fmt_version}."
      )
    endif()

    # The projects intentionally use different include directory names while
    # sharing fmt's ABI. Make this project's headers visible through the one
    # parent provider without installing or compiling another fmt library.
    target_include_directories(
      ${fmt_provider_target} INTERFACE
      $<BUILD_INTERFACE:${fmt_source_dir}/include>
    )

    set(fmt_owned_library "")
    message(
      STATUS
      "${PROJECT_NAME}: reusing parent fmt ${parent_fmt_version}"
    )
  endif()

  set(${DARMA_FMT_OUT_LIBRARY} "${fmt_owned_library}" PARENT_SCOPE)
  set(${DARMA_FMT_OUT_LINK_TARGET} fmt::fmt PARENT_SCOPE)
endfunction()
