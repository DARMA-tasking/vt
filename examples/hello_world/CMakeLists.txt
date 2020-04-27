
set(
  HELLO_WORLD_EXAMPLES
  hello_world
  hello_world_functor
  hello_world_collection
  hello_world_collection_collective
  hello_world_collection_staged_insert
  hello_world_collection_reduce
  hello_world_virtual_context
  hello_world_virtual_context_remote
  ring
  param
  objgroup
)

foreach(EXAMPLE_NAME ${HELLO_WORLD_EXAMPLES})
  # message("Example: building hello_world example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
