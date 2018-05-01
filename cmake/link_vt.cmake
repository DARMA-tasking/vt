
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
    LINK_OPENMP
    LINK_STDTHREAD
    LINK_GTEST
    # the following linking options are enabled by default
    LINK_ATOMIC
    LINK_MPI
    LINK_FMT
    LINK_ZLIB
    LINK_FCONTEXT
    LINK_CHECKPOINT
    LINK_MELD
    LINK_DETECTOR
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

  if (NOT DEFINED ARG_LINK_MPI AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_MPI)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: MPI=${ARG_LINK_MPI}")
    endif()

    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} MPI::MPI_CXX
    )
  endif()

  if (NOT DEFINED ARG_LINK_FCONTEXT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_FCONTEXT)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: fcontext=${ARG_LINK_FCONTEXT}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${FCONTEXT_LIBRARY}
    )
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
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} fmt::fmt
    )
  endif()

  if (NOT DEFINED ARG_LINK_CHECKPOINT AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_CHECKPOINT)
    if (${VT_HAS_SERIALIZATION_LIBRARY})
      if (${ARG_DEBUG_LINK})
        message(STATUS "link_target_with_vt: checkpoint=${ARG_LINK_CHECKPOINT}")
      endif()
      target_link_libraries(
        ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} ${CHECKPOINT_LIBRARY}
      )
    else()
      message(FATAL_ERROR "Trying to link with nonexistent checkpoint library")
    endif()
  endif()

  if (NOT DEFINED ARG_LINK_MELD AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_MELD)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: meld=${ARG_LINK_MELD}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} vt::lib::meld
    )
    # set_target_properties(
    #   ${VIRTUAL_TRANSPORT_LIBRARY}
    #   PROPERTIES INTERFACE_LINK_LIBRARIES vt::lib::meld
    # )
  endif()

  if (NOT DEFINED ARG_LINK_DETECTOR AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_DETECTOR)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: detector=${ARG_LINK_DETECTOR}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} vt::lib::detector
    )
  endif()

  if (NOT DEFINED ARG_LINK_OPENMP AND DEFAULT_THREADING STREQUAL openmp OR ARG_LINK_OPENMP)
    if (${ARG_DEBUG_LINK})
      message(
        STATUS
        "link_target_with_vt: dt=${DEFAULT_THREADING}, omp=${ARG_LINK_OPENMP}"
      )
    endif()
    target_link_libraries(
      ${ARG_TARGET} PUBLIC ${ARG_BUILD_TYPE} OpenMP::OpenMP_CXX
    )
  elseif (NOT DEFINED ARG_LINK_STDTHREAD AND ${ARG_DEFAULT_LINK_SET} OR ARG_LINK_STDTHREAD)
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt(..): stdthread=${ARG_LINK_STDTHREAD}")
    endif()
    # @todo: is there something that needs to be done for std::threads to work
    # in all cases, perhaps "-pthread"?
  endif()

  if (${ARG_LINK_GTEST})
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: gtest=${ARG_LINK_GTEST}")
    endif()
    target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_BUILD_TYPE} GTest::GTest)
    target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_BUILD_TYPE} GTest::Main)
  endif()

  if (${ARG_CUSTOM_LINK_ARGS})
    if (${ARG_DEBUG_LINK})
      message(STATUS "link_target_with_vt: custom=${ARG_CUSTOM_LINK_ARGS}")
    endif()
    target_link_libraries(
      ${ARG_TARGET} PRIVATE ${ARG_BUILD_TYPE} ${ARG_CUSTOM_LINK_ARGS}
     )
  endif()
endfunction()
