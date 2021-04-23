#
# Quirk-detection between different compilers.
#

function(check_cc_quirk VAR CC_FILE)
  if(NOT DEFINED ${VAR})
    set(check_cc_result FALSE)
    set(check_cc_quirk_output "")

    try_compile(
      check_cc_result
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cc_tests/${CC_FILE}
      OUTPUT_VARIABLE check_cc_quirk_output
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
    )

    if(NOT check_cc_result AND VT_DEBUG_CMAKE)
      message(
        STATUS
        ${check_cc_quirk_output}
      )
    endif()

    # quirk'ed is negative check
    mark_as_advanced(${VAR})
    if(check_cc_result)
      set(${VAR} FALSE CACHE INTERNAL "check_cc_quirk result for ${CC_FILE}")
    else()
      set(${VAR} TRUE CACHE INTERNAL "check_cc_quirk result for ${CC_FILE}")
    endif()
  endif()
endfunction()

check_cc_quirk(
  vt_quirked_trivially_copyable_on_msg
  trivially_copyable_with_const_member.cc
)

check_cc_quirk(
  vt_quirked_serialize_method_detection
  declval_on_templated_method_without_instantiation.cc
)

if(vt_quirked_trivially_copyable_on_msg)
  message(
    STATUS
    "Weakened message contract checking.\
 Trivially copyable checks are disabled as a type with a const member not detected as trivially copyable.\
 Compile with another compiler (such as Clang) for increased static validation."
  )
endif()

if(vt_quirked_serialize_method_detection)
  message(
    STATUS
    "Weakened message contract checking.\
 Messages cannot be detected for correct presense (or absense) of a 'serialize' method.\
 Compile with another compiler (such as Clang) for increased static validation."
  )
endif()
