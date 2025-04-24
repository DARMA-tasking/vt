
set(
  COLLECTION_EXAMPLES
  lb_iter
  jacobi1d_vt
  jacobi2d_vt
  jacobi3d_vt
  migrate_collection
  polymorphic_collection
  insertable_collection
  reduce_integral
  transpose
  4d_collection
  matrix
  matrix-comm
)
if (vt_perf_enabled AND vt_perf_found)
  list(APPEND COLLECTION_EXAMPLES do_flops_perf)
endif()

foreach(EXAMPLE_NAME ${COLLECTION_EXAMPLES})
  # message("Example: building collection example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
