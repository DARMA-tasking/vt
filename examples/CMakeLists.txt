
#
# Examples
#

set(
  PROJECT_EXAMPLES_LIST
  tutorial
)


macro(add_example example_name)
  set(EXAMPLE_FILE "${example_name}.cc")

  # if (vt_serialize_static_checker)
  #   set(EXAMPLE_GENERATED_FILE "${CMAKE_CURRENT_BINARY_DIR}/${example_name}_generated.cc")
  #   add_custom_command(
  #     COMMENT "Generated static serialize checks for ${EXAMPLE_FILE}"
  #     OUTPUT ${EXAMPLE_GENERATED_FILE}
  #     COMMAND ${vt_serialize_static_checker} -main -o ${EXAMPLE_GENERATED_FILE} -p ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_FILE}
  #     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_FILE}
  #     VERBATIM
  #   )
  #   #add_custom_target(${EXAMPLE_GENERATED_FILE} DEPENDS ${EXAMPLE_FILE})
  #   add_executable(${example_name}-checked ${EXAMPLE_GENERATED_FILE})
  #   link_target_with_vt(
  #     TARGET ${example_name}-checked
  #     DEFAULT_LINK_SET
  #   )

  #   if (BUILD_TESTING)
  #     add_test_for_example_vt(
  #       ${example_name}-checked
  #       ${EXAMPLE_GENERATED_FILE}
  #       example_tests
  #     )
  #   endif()
  # endif()


  add_executable(${example_name} ${EXAMPLE_FILE})

  add_executable(${example_name}-checked "${example_name}_generated.cc")
  add_dependencies(examples ${example_name}-checked)

  generate_static_checker(${example_name}-checked ${example_name})

  link_target_with_vt(
    TARGET ${example_name}-checked
    DEFAULT_LINK_SET
  )

  link_target_with_vt(
    TARGET ${example_name}
    DEFAULT_LINK_SET
  )

  if (BUILD_TESTING)
    add_test_for_example_vt(
      ${example_name}-checked
      "${example_name}_generated.cc"
      example_tests
    )
  endif()
endmacro()

foreach(EXAMPLE_NAME ${PROJECT_EXAMPLES_LIST})
  # message("Example: building example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()

add_subdirectory(callback)
add_subdirectory(collection)
add_subdirectory(group)
add_subdirectory(hello_world)
add_subdirectory(rdma)
add_subdirectory(termination)
