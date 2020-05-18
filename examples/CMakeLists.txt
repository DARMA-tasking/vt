
#
# Examples
#

set(
  PROJECT_EXAMPLES_LIST
  tutorial
)

macro(add_example example_name)
  set(EXAMPLE_FILE "${example_name}.cc")

  add_executable(${example_name} ${EXAMPLE_FILE})
  add_dependencies(examples ${example_name})

  link_target_with_vt(
    TARGET ${example_name}
    DEFAULT_LINK_SET
  )

  if (BUILD_TESTING)
    add_test_for_example_vt(
      ${example_name}
      ${EXAMPLE_FILE}
      example_tests
    )
  endif()
endmacro()

add_subdirectory(callback)
add_subdirectory(collection)
add_subdirectory(group)
add_subdirectory(hello_world)
add_subdirectory(rdma)
add_subdirectory(termination)
