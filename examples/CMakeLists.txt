
set(
  PROJECT_EXAMPLES_LIST
  hello_world ring rdma_simple_get rdma_simple_get_direct rdma_simple_put
  rdma_simple_put_direct rdma_simple_channel rdma_channel_sync rdma_collective
  callback callback_new reinstate tagged_handler param hello_world_functor
  test_seq trace_ring location_test location_test_msg broadcast_test index_test
  hello_world_vrtContext hello_world_vc jacobi1d_recur_vc jacobi1d_node
  term_epoch serialize_example hello_world_vc_remote hello_world_start
  hello_world_collection rdma_unsized_collection group_example_1 reduce
)

foreach(EXAMPLE ${PROJECT_EXAMPLES_LIST})
  # message("Example: building example >>>>> ${EXAMPLE}")

  if(CMAKE_NO_BUILD_EXAMPLES)
    message("Skipping example: ${EXAMPLE}")
  else()
    add_executable(${EXAMPLE} ${EXAMPLE}.cc)

    include_directories(${EXAMPLE} ${CMAKE_SOURCE_DIR}/src)
    include_directories(${EXAMPLE} ${CMAKE_SOURCE_DIR}/lib/context/src)
    include_directories(${EXAMPLE} ${CMAKE_BINARY_DIR})

    add_dependencies(examples ${EXAMPLE})

    add_test_for_example_vt(
      ${EXAMPLE}
      ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}
      gtest
    )

    target_link_libraries(
      ${EXAMPLE}
      ${VIRTUAL_TRANSPORT_LIBRARY}
      ${ZLIB_LIBRARIES}
      ${MPI_C_LIBRARIES}
      ${FCONTEXT_LIBRARY}
      ${SERIALIZATION_LIBRARY}
    )
  endif()
endforeach()

