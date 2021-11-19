include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
  check_cxx_compiler_flag("${flag}" flag_supported)
  if(flag_supported)
    target_compile_options(${vt_target} PRIVATE ${flag})
  endif()
  unset(flag_supported CACHE)
endfunction()

function(turn_on_warnings vt_target)
  if(NOT hasParent)
    enable_cxx_compiler_flag_if_supported("-Wall")
    enable_cxx_compiler_flag_if_supported("-pedantic")
    enable_cxx_compiler_flag_if_supported("-Wshadow")
    enable_cxx_compiler_flag_if_supported("-Wno-unknown-pragmas")
    enable_cxx_compiler_flag_if_supported("-Wsign-compare")
    # Not really a warning, is still diagnostic related..
    enable_cxx_compiler_flag_if_supported("-ftemplate-backtrace-limit=100")

    if (vt_werror_enabled)   # Treat warning as errors
      enable_cxx_compiler_flag_if_supported("-Werror")
    endif()
  endif()
endfunction()
