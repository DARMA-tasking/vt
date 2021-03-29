
function(check_compile VAR CC_FILE)
  if(NOT DEFINED ${VAR})
    set(check_cc_result FALSE)
    set(check_cc_output "")

    try_compile(
      check_cc_result
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/${CC_FILE}
      OUTPUT_VARIABLE check_cc_output
      CXX_STANDARD ${CMAKE_CXX_STANDARD}
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
    )

    ## message(STATUS ${check_cc_output})

    mark_as_advanced(${VAR})

    if(check_cc_result)
      set(${VAR} TRUE CACHE INTERNAL "check_compile result for ${CC_FILE}")
    else()
      set(${VAR} FALSE CACHE INTERNAL "check_compile result for ${CC_FILE}")
    endif()
  endif()
endfunction()

check_compile(
  context_valgrind_stack_compiled
  valgrind_stack_register.cmake.cc
)

if (context_valgrind_stack_compiled)
  message(
    STATUS "Successfully compiled with valgrind stack registration"
  )
else()
  message(
    STATUS "Failed to compile with valgrind stack registration"
  )
endif()
