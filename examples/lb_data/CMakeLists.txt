
set(
  LB_DATA_EXAMPLES
  lb_data_file_generator
)

foreach(EXAMPLE_NAME ${LB_DATA_EXAMPLES})
  add_example(${EXAMPLE_NAME})
endforeach()
