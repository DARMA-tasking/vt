
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
    TARGET_NAME
    TARGET_WORKING_DIRECTORY
    WRAPPER_EXECUTABLE
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

  add_test(
    NAME                ${ARG_TARGET_NAME}
    WORKING_DIRECTORY   ${ARG_TARGET_WORKING_DIRECTORY}
    COMMAND
      ${MPI_RUN_COMMAND}
      ${MPI_NUMPROC_FLAG} ${ARG_TARGET_NPROC}
      ${MPI_PRE_FLAGS} ${MPI_EXTRA_FLAGS_LIST}
      ${ARG_WRAPPER_EXECUTABLE} ${ARG_WRAPPER_ARGS} ./${ARG_TARGET_EXECUTABLE}
      ${MPI_EPI_FLAGS} ${ARG_TARGET_ARGS}
  )

  set_tests_properties(
    ${ARG_TARGET_NAME}
    PROPERTIES TIMEOUT 60
    PROCESSORS ${ARG_TARGET_NPROC}
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

  set(CUR_N_PROC "1")
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
macro(add_test_for_example_vt test_target test_exec test_list)
  foreach(PROC ${PROC_TEST_LIST})
    get_filename_component(test_name ${test_exec} NAME_WE)

    # Examples run with additional flags per enabled build options
    # when such can be applied generally. This does not cover specific
    # interactions between various combinations.
    set(EXEC_ARGS ${ARGN})
    if (vt_trace_enabled)
        list(APPEND EXEC_ARGS "--vt_trace")
    endif()

    run_executable_with_mpi(
      TARGET_EXECUTABLE            ${test_name}
      TARGET_ARGS                  ${EXEC_ARGS}
      TARGET_NPROC                 ${PROC}
      TARGET_NAME                  vt_example:${test_name}_${PROC}
      TARGET_WORKING_DIRECTORY     ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(
      vt_example:${test_name}_${PROC}
      PROPERTIES
      FAIL_REGULAR_EXPRESSION "Segmentation fault"
      LABELS "example"
    )
  endforeach()
endmacro()
