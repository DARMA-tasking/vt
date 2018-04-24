
function(run_executable_with_mpi)
  if (ARGC LESS 1)
    message(FATAL_ERROR "no arguments supplied to run_executable_with_mpi")
  endif()

  set(
    noValOption
    EXECUTE_WITH_WRAPPER
  )
  set(
    singleValArg
    TARGET_EXECUTABLE
    TARGET_NPROC
    WRAPPER_EXECUTABLE
    VARIABLE_OUT
  )
  set(
    multiValueArg
    TARGET_ARGS
    WRAPPER_ARGS
  )
  set(allKeywords ${noValOption} ${singleValArg} ${multiValueArg})

  cmake_parse_arguments(
    ARG "${noValOption}" "${singleValArg}" "${multiValueArgs}" ${ARGN}
  )

  if (NOT DEFINED ARG_EXECUTE_WITH_WRAPPER)
    set(ARG_WRAPPER_EXECUTABLE "")
    set(ARG_WRAPPER_ARGS "")
  elseif(ARG_WRAPPER_EXECUTABLE STREQUAL "")
    message(
      FATAL_ERROR
      "Executable must be specified for wrapper (WRAPPER_EXECUTABLE)"
    )
  endif()

  if (NOT DEFINED ${ARG_VARIABLE_OUT})
    message(
      FATAL_ERROR
      "Out variable where string is generated must be defined (VARIABLE_OUT)"
    )
  endif()

  set(
    ${ARG_VARIABLE_OUT}
    "${MPI_RUN_COMMAND}                                                      \
     ${MPI_NUMPROC_FLAG} ${ARG_TARGET_NPROC}                                 \
     ${MPI_PRE_FLAGS}                                                        \
     ${ARG_WRAPPER_EXECUTABLE} ${ARG_WRAPPER_ARGS} ${ARG_TARGET_EXECUTABLE}  \
     ${MPI_EPI_FLAGS} ${ARG_TARGET_ARGS}                                     \
    "
    PARENT_SCOPE
  )
endfunction()

function(build_mpi_proc_test_list)
  if (ARGC LESS 1)
    message(FATAL_ERROR "no arguments supplied to build_mpi_proc_test_list")
  endif()

  set(noValOption)
  set(singleValArg MAX_PROC VARIABLE_OUT )
  set(multiValueArg)
  set(allKeywords ${noValOption} ${singleValArg} ${multiValueArg})
  cmake_parse_arguments(
    ARG "${noValOption}" "${singleValArg}" "${multiValueArgs}" ${ARGN}
  )

  if (NOT DEFINED ARG_MAX_PROC)
    # Default to 8 processors
    set(ARG_MAX_PROC "8")
  elseif(${ARG_MAX_PROC} LESS "2")
    # The min number of processors allowed in the list is 2
    set(ARG_MAX_PROC "2")
  endif()

  if (NOT DEFINED ${ARG_VARIABLE_OUT})
    message(
      FATAL_ERROR
      "Out variable where string is generated must be defined (VARIABLE_OUT)"
    )
  endif()

  # Start with 2 processors since many tests/examples require at least 2
  set(CUR_N_PROC "2")
  set(CUR_PROC_LIST "")
  while(CUR_N_PROC LESS_EQUAL "${ARG_MAX_PROC}")
    #message("${CUR_N_PROC}")
    list(APPEND CUR_PROC_LIST ${CUR_N_PROC})
    math(EXPR NEW_VAL "${CUR_N_PROC} * 2")
    set(CUR_N_PROC "${NEW_VAL}")
  endwhile()

  set(${ARG_VARIABLE_OUT} "${CUR_PROC_LIST}" PARENT_SCOPE)
endfunction()

# Set up test scaffolding for running examples
macro(add_test_for_example_vt test_name test_exec)
  foreach(PROC ${PROC_TEST_LIST})
    #message("Adding test for example: ${test_name}")
    set(TEST_STRING "")
   run_executable_with_mpi(
      TARGET_EXECUTABLE ${test_exec}
      TARGET_ARGS       ${ARGN}
      TARGET_NPROC      ${PROC}
      VARIABLE_OUT      TEST_STRING
    )
   add_test(${test_name}_${PROC} ${TEST_STRING})
   set_tests_properties(
      ${test_name}_${PROC}
      PROPERTIES TIMEOUT 300 FAIL_REGULAR_EXPRESSION "FAILED;WARNING"
    )
  endforeach()
endmacro()
