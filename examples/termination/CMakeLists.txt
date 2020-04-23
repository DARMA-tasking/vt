
set(
  TERMINATION_EXAMPLES
  termination_collective
  termination_rooted
)

foreach(EXAMPLE_NAME ${TERMINATION_EXAMPLES})
  # message("Example: building term example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
