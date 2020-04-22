
set(
  PROJECT_EXAMPLES_LIST
  hello_world
  ring
  rdma_simple_get
  rdma_simple_get_direct
  rdma_simple_put
  rdma_simple_put_direct
  rdma_simple_channel
  rdma_channel_sync
  rdma_collective
  ctx_callback
  param
  hello_world_functor
  test_seq
  term_epoch
  term_epoch_advanced
  term_ds
  trace_ring
  location_test
  location_test_msg
  lb_iter
  hello_world_vrtContext
  hello_world_vc
  jacobi1d_recur_vc
  jacobi1d_node
  jacobi1d_vt
  jacobi2d_vt
  serialize_example
  hello_world_vc_remote
  hello_world_collection
  simple_collection
  simple_collection_collective
  simple_collection_insert
  simple_collection_reduce
  collection_insert
  rdma_unsized_collection
  group_rooted
  group_collective
  reduce
  transpose
  tutorial
  objgroup
)

#
# Examples
#

  foreach(EXAMPLE_NAME ${PROJECT_EXAMPLES_LIST})
    # message("Example: building example >>>>> ${EXAMPLE}")

    set(EXAMPLE_FILE "${EXAMPLE_NAME}.cc")

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    add_dependencies(examples ${EXAMPLE_NAME})

    link_target_with_vt(
      TARGET ${EXAMPLE_NAME}
      DEFAULT_LINK_SET
    )

    if (BUILD_TESTING)
      add_test_for_example_vt(
        ${EXAMPLE_NAME}
        ${EXAMPLE_FILE}
        example_tests
      )
    endif()

  endforeach()


