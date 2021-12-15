function(config_for_openmp)
  message(
    STATUS
    "OpenMP has been found: "
    "Linker=\"${OpenMP_EXE_LINKER_FLAGS}\", "
    "CC FLAGS=\"${OpenMP_C_FLAGS}\", "
    "CXX FLAGS=\"${OpenMP_CXX_FLAGS}\""
  )

  set(DEFAULT_THREADING openmp PARENT_SCOPE)

  set(vt_feature_cmake_openmp "1" PARENT_SCOPE)
  set(vt_feature_cmake_stdthread "0" PARENT_SCOPE)
  set(vt_fcontext_enabled "0" PARENT_SCOPE)

  set(LOCAL_THREADS_DEPENDENCY "find_dependency(OpenMP REQUIRED)" PARENT_SCOPE)
endfunction(config_for_openmp)

function(config_for_std_thread)
  set(DEFAULT_THREADING stdthread PARENT_SCOPE)

  set(vt_feature_cmake_openmp "0" PARENT_SCOPE)
  set(vt_feature_cmake_stdthread "1" PARENT_SCOPE)
  set(vt_fcontext_enabled "0" PARENT_SCOPE)

  set(LOCAL_THREADS_DEPENDENCY "find_dependency(Threads REQUIRED)" PARENT_SCOPE)
endfunction(config_for_std_thread)

function(config_for_fcontext)
  set(vt_feature_cmake_openmp "0" PARENT_SCOPE)
  set(vt_feature_cmake_stdthread "0" PARENT_SCOPE)
  set(vt_fcontext_enabled "1" PARENT_SCOPE)
endfunction(config_for_fcontext)

function(config_no_threading)
  set(vt_feature_cmake_openmp "0" PARENT_SCOPE)
  set(vt_feature_cmake_stdthread "0" PARENT_SCOPE)
  set(vt_fcontext_enabled "0" PARENT_SCOPE)
endfunction(config_no_threading)
