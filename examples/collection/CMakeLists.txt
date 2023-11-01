
set(
  COLLECTION_EXAMPLES
  lb_iter
  jacobi1d_vt
  jacobi1d_vt_sync
  jacobi2d_vt
  jacobi2d_vt_sync
  migrate_collection
  polymorphic_collection
  insertable_collection
  reduce_integral
  transpose
)

foreach(EXAMPLE_NAME ${COLLECTION_EXAMPLES})
  # message("Example: building collection example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
