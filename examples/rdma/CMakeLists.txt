
set(
  RDMA_EXAMPLES
  rdma_simple_get
  rdma_simple_get_direct
  rdma_simple_put
  rdma_simple_put_direct
)

foreach(EXAMPLE_NAME ${RDMA_EXAMPLES})
  # message("Example: building rdma example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
