include(CheckCXXCompilerFlag)

macro(add_cxx_compiler_flag_if_supported flag)
  check_cxx_compiler_flag("${flag}" flag_supported)
  if(flag_supported)
    list(APPEND VT_WARNING_FLAGS "${flag}")
  endif()
  unset(flag_supported CACHE)
endmacro()

if(NOT DEFINED VT_WARNING_FLAGS)
  add_cxx_compiler_flag_if_supported("-Wall")
  add_cxx_compiler_flag_if_supported("-pedantic")
  add_cxx_compiler_flag_if_supported("-Wshadow")
  add_cxx_compiler_flag_if_supported("-Wno-unknown-pragmas")
  add_cxx_compiler_flag_if_supported("-Wsign-compare")
  # Not really a warning, is still diagnostic related..
  add_cxx_compiler_flag_if_supported("-ftemplate-backtrace-limit=100")

  if (vt_werror_enabled)   # Treat warning as errors
  add_cxx_compiler_flag_if_supported("-Werror")
  endif()
endif()

set(
  VT_WARNING_FLAGS ${VT_WARNING_FLAGS} CACHE INTERNAL
  "Project's warning options")

macro(turn_on_warnings vt_target)
  target_compile_options(${vt_target} PRIVATE ${VT_WARNING_FLAGS})
endmacro()
