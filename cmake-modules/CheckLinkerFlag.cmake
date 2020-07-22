include(CheckCXXCompilerFlag)

if(POLICY CMP0056)
  # Honor link flags in try_compile() source-file signature.
  cmake_policy(SET CMP0056 NEW)
endif()

# This is written because cmake does not support `check_linker_flag` until 3.18
function(check_linker_flag flag out_var)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flag}")
  check_cxx_compiler_flag("" ${out_var})
endfunction()
