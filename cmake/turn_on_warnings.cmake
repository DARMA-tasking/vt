include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
  string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
    check_cxx_compiler_flag("${flag}" flag_supported)
    if(flag_supported)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
    unset(flag_supported CACHE)
  endif()
endfunction()

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
