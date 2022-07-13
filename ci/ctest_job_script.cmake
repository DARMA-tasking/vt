include(${CTEST_SCRIPT_DIRECTORY}/CTestConfig.cmake)

site_name(CTEST_SITE)
set(CTEST_BUILD_NAME
    "${CMAKE_HOST_SYSTEM_NAME}-$ENV{CI_JOB_NAME}"
)
set(CTEST_SOURCE_DIRECTORY
    "$ENV{VT}"
)
set(CTEST_BINARY_DIRECTORY
    "$ENV{VT_BUILD}"
)

if ( NOT DEFINED ENV{VT_TRACE_RUNTIME_ENABLED} )
  set(ENV{VT_TRACE_RUNTIME_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_LB_ENABLED} )
  set(ENV{VT_LB_ENABLED} "1")
endif()
if ( NOT DEFINED ENV{VT_TRACE_ENABLED} )
  set(ENV{VT_TRACE_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_BUILD_TRACE_ONLY} )
  set(ENV{VT_BUILD_TRACE_ONLY} "0")
endif()
if ( NOT DEFINED ENV{VT_DOXYGEN_ENABLED} )
  set(ENV{VT_DOXYGEN_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_MIMALLOC_ENABLED} )
  set(ENV{VT_MIMALLOC_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_ASAN_ENABLED} )
  set(ENV{VT_ASAN_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_UBSAN_ENABLED} )
  set(ENV{VT_UBSAN_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_WERROR_ENABLED} )
  set(ENV{VT_WERROR_ENABLED} "0" )
endif()
if ( NOT DEFINED ENV{VT_POOL_ENABLED} )
  set(ENV{VT_POOL_ENABLED} "1")
endif()
if ( NOT DEFINED ENV{VT_EXTENDED_TESTS_ENABLED} )
  set(ENV{VT_EXTENDED_TESTS_ENABLED} "1")
endif()
if ( NOT DEFINED ENV{VT_ZOLTAN_ENABLED} )
  set(ENV{VT_ZOLTAN_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_PRODUCTION_BUILD_ENABLED} )
  set(ENV{VT_PRODUCTION_BUILD_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_UNITY_BUILD_ENABLED} )
  set(ENV{VT_UNITY_BUILD_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_DIAGNOSTICS_ENABLED} )
  set(ENV{VT_DIAGNOSTICS_ENABLED} "1")
endif()
if ( NOT DEFINED ENV{VT_DIAGNOSTICS_RUNTIME_ENABLED} )
  set(ENV{VT_DIAGNOSTICS_RUNTIME_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_FCONTEXT_ENABLED} )
  set(ENV{VT_FCONTEXT_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{VT_FCONTEXT_BUILD_TESTS_EXAMPLES} )
  set(ENV{VT_FCONTEXT_BUILD_TESTS_EXAMPLES} "0")
endif()
if ( NOT DEFINED ENV{VT_RDMA_TESTS_ENABLED} )
  set(ENV{VT_RDMA_TESTS_ENABLED} "1")
endif()
if ( NOT DEFINED ENV{VT_USE_OPENMP} )
  set(ENV{VT_USE_OPENMP} "0")
endif()
if ( NOT DEFINED ENV{VT_USE_STD_THREAD} )
  set(ENV{VT_USE_STD_THREAD} "0")
endif()
if ( NOT DEFINED ENV{CODE_COVERAGE} )
  set(ENV{CODE_COVERAGE} "0")
endif()
if ( NOT DEFINED ENV{VT_MPI_GUARD_ENABLED} )
  set(ENV{VT_MPI_GUARD_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{CMAKE_BUILD_TYPE} )
  set(ENV{CMAKE_BUILD_TYPE} "Release")
endif()
if ( NOT DEFINED ENV{VT_CI_BUILD} )
  set(ENV{VT_CI_BUILD} "0")
endif()
if ( NOT DEFINED ENV{VT_NO_COLOR_ENABLED} )
  set(ENV{VT_NO_COLOR_ENABLED} "0")
endif()
if ( NOT DEFINED ENV{BUILD_SHARED_LIBS:} )
  set(ENV{BUILD_SHARED_LIBS} "0")
endif()

set(CTEST_CMAKE_GENERATOR    Ninja)
set(CTEST_CONFIGURATION_TYPE $ENV{CMAKE_BUILD_TYPE} )

set(configureOpts
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=1"
    "-Dvt_test_trace_runtime_enabled=$ENV{VT_TRACE_RUNTIME_ENABLED}"
    "-Dvt_lb_enabled=$ENV{VT_LB_ENABLED}"
    "-Dvt_trace_enabled=$ENV{VT_TRACE_ENABLED}"
    "-Dvt_trace_only=$ENV{VT_BUILD_TRACE_ONLY}"
    "-Dvt_doxygen_enabled=$ENV{VT_DOXYGEN_ENABLED}"
    "-Dvt_mimalloc_enabled=$ENV{VT_MIMALLOC_ENABLED}"
    "-Dvt_asan_enabled=$ENV{VT_ASAN_ENABLED}"
    "-Dvt_ubsan_enabled=$ENV{VT_UBSAN_ENABLED}"
    "-Dvt_werror_enabled=$ENV{VT_WERROR_ENABLED}"
    "-Dvt_pool_enabled=$ENV{VT_POOL_ENABLED}"
    "-Dvt_build_extended_tests=$ENV{VT_EXTENDED_TESTS_ENABLED}"
    "-Dvt_zoltan_enabled=$ENV{VT_ZOLTAN_ENABLED}"
    "-Dvt_production_build_enabled=$ENV{VT_PRODUCTION_BUILD_ENABLED}"
    "-Dvt_unity_build_enabled=$ENV{VT_UNITY_BUILD_ENABLED}"
    "-Dvt_diagnostics_enabled=$ENV{VT_DIAGNOSTICS_ENABLED}"
    "-Dvt_diagnostics_runtime_enabled=$ENV{VT_DIAGNOSTICS_RUNTIME_ENABLED}"
    "-Dvt_fcontext_enabled=$ENV{VT_FCONTEXT_ENABLED}"
    "-Dvt_fcontext_build_tests_examples=$ENV{VT_FCONTEXT_BUILD_TESTS_EXAMPLES}"
    "-Dvt_rdma_tests_enabled=$ENV{VT_RDMA_TESTS_ENABLED}"
    "-DUSE_OPENMP=$ENV{VT_USE_OPENMP}"
    "-DUSE_STD_THREAD=$ENV{VT_USE_STD_THREAD}"
    "-DCODE_COVERAGE=$ENV{CODE_COVERAGE}"
    "-DMI_INTERPOSE:BOOL=ON"
    "-DMI_OVERRIDE:BOOL=ON"
    "-Dvt_mpi_guards=$ENV{VT_MPI_GUARD_ENABLED}"
    "-DMPI_EXTRA_FLAGS=$ENV{MPI_EXTRA_FLAGS}"
    "-DCMAKE_BUILD_TYPE=$ENV{CMAKE_BUILD_TYPE}"
    "-DMPI_C_COMPILER=$ENV{MPICC}"
    "-DMPI_CXX_COMPILER=$ENV{MPICXX}"
    "-DCMAKE_CXX_COMPILER=$ENV{CXX}"
    "-DCMAKE_C_COMPILER=$ENV{CC}"
    "-DCMAKE_EXE_LINKER_FLAGS=$ENV{CMAKE_EXE_LINKER_FLAGS}"
    "-Ddetector_DIR=$ENV{DETECTOR_BUILD}/install"
    "-Dcheckpoint_DIR=$ENV{CHECKPOINT_BUILD}/install"
    "-DCMAKE_PREFIX_PATH=$ENV{CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX=$ENV{VT_BUILD}/install"
    "-Dvt_ci_build=$ENV{VT_CI_BUILD}"
    "-Dvt_debug_verbose=$ENV{VT_DEBUG_VERBOSE}"
    "-Dvt_tests_num_nodes=$ENV{VT_TESTS_NUM_NODES}"
    "-Dvt_no_color_enabled=$ENV{VT_NO_COLOR_ENABLED}"
    "-DBUILD_SHARED_LIBS=$ENV{BUILD_SHARED_LIBS}"
    "-DCMAKE_JOB_POOLS=default_pool=$ENV{parallel_level}"
    "-DCMAKE_JOB_POOL_COMPILE='default_pool'"
    "-DCMAKE_JOB_POOL_LINK='default_pool'"
)
#ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
ctest_start(Continuous)
ctest_configure(OPTIONS "${configureOpts}" RETURN_VALUE ret_conf)
ctest_submit(PARTS Start Configure)
if (NOT ret_conf)
  ctest_build(RETURN_VALUE ret_build)
  ctest_submit(PARTS Build)
# ctest_upload(FILES
#     ${CTEST_BINARY_DIRECTORY}/mytest.log
#     ${CTEST_BINARY_DIRECTORY}/anotherFile.txt
# )
# ctest_submit(PARTS Upload Submit)
  if (NOT ret_build)
    ctest_test(PARALLEL_LEVEL $ENV{parallel_level})
    ctest_submit(PARTS Test)
  endif()
endif()
if(NOT CMAKE_VERSION VERSION_LESS "3.14")
    ctest_submit(PARTS Done)
endif()