
set(
  GROUP_EXAMPLES
  group_rooted
  group_collective
)

foreach(EXAMPLE_NAME ${GROUP_EXAMPLES})
  # message("Example: building group example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
