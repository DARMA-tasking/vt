# Convenience function for linking VT or with VT
function(link_target_with_vt)
  if (ARGC LESS 1)
    message(FATAL_ERROR "no arguments supplied to link_target_with_vt(..)")
  endif()

  set(
    noValOption
    # link with the default set of libraries associated with VT
    DEFAULT_LINK_SET
    # turn all the debugging prints on
    DEBUG_LINK
    # turn on for linking VT itself
    LINK_VT_LIB
  )
  set(
    singleValArg
    TARGET
    BUILD_TYPE
    LINK_GTEST
    # the following linking options are enabled by default
    LINK_ATOMIC
    LINK_MPI
    LINK_FMT
    LINK_UNWIND
    LINK_ENG_FORMAT
    LINK_ZLIB
    LINK_FCONTEXT
    LINK_CHECKPOINT
    LINK_CLI11
    LINK_DL
    LINK_ZOLTAN
    LINK_FORT
    LINK_JSON
    LINK_BROTLI
  )
  set(
    multiValueArg
    CUSTOM_LINK_ARGS
  )
  set(allKeywords ${noValOption} ${singleValArg} ${multiValueArg})

  cmake_parse_arguments(
    ARG "${noValOption}" "${singleValArg}" "${multiValueArgs}" ${ARGN}
  )

  if (${ARG_DEBUG_LINK})
    message(STATUS "link_target_with_vt(..): argc=${ARGC}")
    message(STATUS "link_target_with_vt: target=${ARG_TARGET}")
    message(STATUS "link_target_with_vt: default link=${ARG_DEFAULT_LINK_SET}")
  endif()

  if (NOT DEFINED ARG_LINK_JSON AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_JSON)
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${JSON_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_BROTLI AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_BROTLI)
    target_link_libraries(
      ${ARG_TARGET} PRIVATE ${ARG_BUILD_TYPE} ${BROTLI_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_FORT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_FORT)
    if (vt_libfort_enabled)
      target_link_libraries(
        ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${FORT_LIBRARY}
      )
    endif()
  endif()

  if (NOT DEFINED ARG_LINK_ZOLTAN AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_ZOLTAN)
    if (vt_zoltan_enabled)
      if (${ARG_DEBUG_LINK})
        message(STATUS "link_target_with_vt: zoltan=${ARG_LINK_ZOLTAN}")
      endif()

      target_link_libraries(
        ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} zoltan
        )
      target_include_directories(
        ${ARG_TARGET} PUBLIC $<BUILD_INTERFACE:${Zoltan_INCLUDE_DIRS}>
        )
    endif()
  endif()

  if (${ARG_LINK_GTEST})
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: gtest=${ARG_LINK_GTEST}")
    endif()
    target_link_libraries(${ARG_TARGET} PRIVATE gtest)
  endif()

  if (NOT DEFINED ARG_LINK_UNWIND AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_UNWIND)
    if (vt_has_libunwind_h)
      if (${ARG_DEBUG_LINK})
        message(STATUS "link_target_with_vt: unwind=${ARG_LINK_UNWIND}")
      endif()
      if (NOT DEFINED APPLE)
        target_link_libraries(
          ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} unwind
        )
      endif()
    endif()
  endif()

  if (NOT ARG_LINK_VT_LIB)
    # Unconditionally link the VT library for this target unless linking the VT
    # library itself itself
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${VIRTUAL_TRANSPORT_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_ATOMIC AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_ATOMIC)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: atomic=${ARG_LINK_ATOMIC}")
    endif()

    set(ATOMIC_LIB atomic)
    if (LINK_WITH_ATOMIC)
      target_link_libraries(
        ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${ATOMIC_LIB}
      )
    endif()
  endif()

  if (NOT DEFINED ARG_LINK_DL AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_DL)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: dl=${ARG_LINK_DL}")
    endif()

    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${CMAKE_DL_LIBS}
    )
  endif()

  if (NOT DEFINED ARG_LINK_MPI AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_MPI)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: MPI=${ARG_LINK_MPI}")
    endif()

    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} MPI::MPI_CXX
    )
  endif()

  if (${vt_fcontext_enabled})
    if (NOT DEFINED ARG_LINK_FCONTEXT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_FCONTEXT)
      if (${ARG_DEBUG_LINK})
        message(STATUS "link_target_with_vt: fcontext=${ARG_LINK_FCONTEXT}")
      endif()
      target_link_libraries(
        ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${FCONTEXT_LIBRARY}
        )
    endif()
  endif()

  if (NOT DEFINED ARG_LINK_ZLIB AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_ZLIB)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: zlib=${ARG_LINK_ZLIB}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${ZLIB_LIBRARIES}
    )
  endif()

  if (NOT DEFINED ARG_LINK_FMT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_FMT)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: fmt=${ARG_LINK_FMT}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${FMT_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_ENG_FORMAT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_ENG_FORMAT)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: EngFormat-Cpp=${ARG_LINK_ENG_FORMAT}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${ENG_FORMAT_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_CHECKPOINT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_CHECKPOINT)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: checkpoint=${ARG_LINK_CHECKPOINT}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${CHECKPOINT_LIBRARY}
    )
  endif()

  if (NOT DEFINED ARG_LINK_CLI11 AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_CLI11)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: cli11=${ARG_LINK_CLI11}")
    endif()
    target_include_directories(${ARG_TARGET} PUBLIC
      $<BUILD_INTERFACE:${PROJECT_BASE_DIR}/lib/CLI>
      $<INSTALL_INTERFACE:include/CLI>
    )
  endif()

  if (${vt_mimalloc_enabled})
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: mimalloc=${vt_mimalloc_enabled}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${MIMALLOC_LIBRARY}
    )
  endif()

  if (${ARG_CUSTOM_LINK_ARGS})
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: custom=${ARG_CUSTOM_LINK_ARGS}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PRIVATE ${ARG_BUILD_TYPE} ${ARG_CUSTOM_LINK_ARGS}
     )
  endif()

  if (vt_asan_enabled)
    target_link_libraries(${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} -fsanitize=address)
  endif()

  if (vt_ubsan_enabled)
    target_link_libraries(${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} -fsanitize=undefined)
  endif()
endfunction()
