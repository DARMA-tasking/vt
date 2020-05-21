

macro(generate_static_checker target target_file)

  if (vt_serialize_static_checker)
    set(target_cc "${target_file}.cc")

    set(GENERATED_TARGET "${CMAKE_CURRENT_BINARY_DIR}/${target_file}_generated.cc")

    message(STATUS "target=${target} generated=${GENERATED_TARGET}")

    add_custom_command(
      PRE_BUILD
      COMMENT "Generating static serialize checks for ${target_cc}"
      OUTPUT ${GENERATED_TARGET}
      COMMAND ${vt_serialize_static_checker} -o ${GENERATED_TARGET} -main -p ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_SOURCE_DIR}/${target_cc}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${target_cc}
      VERBATIM
    )

    set_source_files_properties(
      ${GENERATED_TARGET} PROPERTIES GENERATED TRUE
    )

    # add_custom_target(
    #   ${target}-generated
    #   DEPENDS ${GENERATED_TARGET}
    #   COMMENT "Checking if re-generation is required for ${GENERATED_TARGET}"
    # )

    # add_dependencies(${target} ${target}-generated)

    #target_compile_options(${target} PUBLIC "--include=${GENERATED_TARGET}")

    #add_custom_target(${GENERATED_TARGET} DEPENDS ${TARGET})

    # add_executable(${target_file}-checked ${GENERATED_TARGET})

    # link_target_with_vt(
    #   TARGET ${target_file}-checked
    #   DEFAULT_LINK_SET
    # )

    # if (BUILD_TESTING)
    #   add_test_for_example_vt(
    #     ${target_file}-checked
    #     ${GENERATED_TARGET}
    #     example_tests
    #   )
    # endif()
  endif()

endmacro()
