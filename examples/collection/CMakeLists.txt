
set(
  COLLECTION_EXAMPLES
  lb_iter
  jacobi1d_vt
  jacobi2d_vt
  migrate_collection
  insertable_collection
  reduce_integral
  transpose
  lb_tuning_driver
)

foreach(EXAMPLE_NAME ${COLLECTION_EXAMPLES})
  # message("Example: building collection example >>>>> ${EXAMPLE_NAME}")

  add_example(${EXAMPLE_NAME})
endforeach()
