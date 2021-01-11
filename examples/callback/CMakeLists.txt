
set(
  CALLBACK_EXAMPLES
  callback
  callback_context
)

foreach(EXAMPLE_NAME ${CALLBACK_EXAMPLES})
  # message("Example: building callback example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
