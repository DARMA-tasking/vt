
# Set up test scaffolding for running examples
macro(add_test_for_example_vt test_name test_exec)
  foreach(PROC ${PROC_TEST_LIST})
    if(CMAKE_NO_BUILD_TESTS)
      message("Skipping test: ${test_name}_${PROC}")
    else()
      #message("Adding test for example: ${test_name}")
      add_test(
        ${test_name}_${PROC}
        ${MPI_RUN_COMMAND} ${MPI_NUMPROC_FLAG} ${PROC}
        ${test_exec} ${ARGN}
      )

      set_tests_properties(
        ${test_name}_${PROC}
        PROPERTIES TIMEOUT 300 FAIL_REGULAR_EXPRESSION "FAILED;WARNING"
      )
    endif()
  endforeach()
endmacro()
