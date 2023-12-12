# need a lower-case subdirectory name to use for the chosen build type
if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "No CMAKE_BUILD_TYPE detected")
    set(lower_CMAKE_BUILD_TYPE "undefined")
else()
    string(TOLOWER ${CMAKE_BUILD_TYPE} lower_CMAKE_BUILD_TYPE)
endif()

# enumerate configurations we will support
if (NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(VT_CONFIG_TYPES ${lower_CMAKE_BUILD_TYPE} "debug" "release" "relwithdebinfo")
else()
    set(VT_CONFIG_TYPES ${lower_CMAKE_BUILD_TYPE})
    foreach(type ${CMAKE_CONFIGURATION_TYPES})
        string(TOLOWER ${type} tmp_lowercase_type)
        list(APPEND VT_CONFIG_TYPES ${tmp_lowercase_type})
    endforeach(type)
    unset(tmp_lowercase_type)
endif()
list(REMOVE_DUPLICATES VT_CONFIG_TYPES)

# - option_name:        The name of the option.
# - cmake_description:  A descriptive message that is printed when the option is enabled or disabled.
# - option_description: A description of the option. This is used by the option() command for documentation purposes.
# - default_value:      The default value of the option (ON or OFF).
# - cmake_var:          The name of the CMake variable that will be set based on the option's value.
function (define_option option_name cmake_description option_description default_value cmake_var)
  option(${option_name} "${option_description}" ${default_value})
  if(${option_name})
    set(${cmake_var} "1" CACHE INTERNAL "")
    message(STATUS "${cmake_description} enabled")
  else()
    set(${cmake_var} "0" CACHE INTERNAL "")
    message(STATUS "${cmake_description} disabled")
  endif()
endfunction()

##########################################################################################################

list(APPEND CMAKE_MESSAGE_INDENT "Building VT with ")

define_option(vt_lb_enabled "load balancing" "Build VT with load balancing enabled" ON vt_feature_cmake_lblite)
define_option(vt_trace_enabled "trace" "Build VT with trace enabled" OFF vt_feature_cmake_trace_enabled)
define_option(vt_trace_only "additional target for VT in trace-only mode"
    "Build VT with additional target for VT in trace-only mode" OFF vt_feature_cmake_trace_only
)
# This will be changed back to "1" for vt-trace config file
set(vt_feature_cmake_trace_only "0")

set(vt_priority_bits_per_level 3 CACHE
    STRING "Number of bits to use per VT priority level"
)
set(vt_feature_cmake_priority_bits_level "${vt_priority_bits_per_level}")
define_option(vt_priorities_enabled
    "priority bits per level: ${vt_priority_bits_per_level}"
    "Build VT with message priorities enabled" OFF vt_feature_cmake_priorities
)
define_option(vt_test_trace_runtime_enabled
    "runtime tracing (for testing)"
    "Build VT with runtime tracing enabled (for testing)" OFF vt_feature_cmake_test_trace_on
)
define_option(vt_pool_enabled "memory pool" "Build VT with memory pool" ON vt_feature_cmake_memory_pool)
define_option(vt_bit_check_overflow "bit check overflow" "Build VT with bit check overflow"
    OFF vt_feature_cmake_bit_check_overflow
)
define_option(vt_fcontext_enabled "fcontext (ULT)" "Build VT with fcontext (ULT) enabled"
    OFF vt_feature_cmake_fcontext
)
define_option(vt_mimalloc_enabled "mimalloc" "Build VT with fcontext (ULT) enabled"
    OFF vt_feature_cmake_mimalloc
)

if (vt_mpi_guards AND NOT PERL_FOUND)
    # No perl? Can't generate wrapper source file.
    message(STATUS "vt_mpi_guards=ON perl not found! Disabling user MPI prevention guards")
    set(vt_mpi_guards OFF)
endif()
if(vt_trace_only)
    set(vt_mpi_guards ON)
endif()

define_option(vt_mpi_guards "user MPI prevention guards"
    "Build VT with poison MPI calls: code invoked from VT callbacks cannot invoke MPI functions"
    ON vt_feature_cmake_mpi_access_guards
)

define_option(vt_zoltan_enabled "Zoltan" "Build VT with Zoltan" OFF vt_feature_cmake_zoltan)

if(NOT LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "release")
    set(default_debug_verbose ON)
else()
    set(default_debug_verbose OFF)
endif()
define_option(vt_debug_verbose "verbose debug printing"
    "Build VT with verbose debug printing enabled" ${default_debug_verbose} vt_feature_cmake_debug_verbose
)
define_option(vt_rdma_tests_enabled "RDMA tests" "Build VT with RDMA tests enabled"
    ON vt_feature_cmake_rdma_tests
)


#####################################################
#################### DIAGNOSTICS ####################
#####################################################
define_option(vt_diagnostics_runtime_enabled "runtime performance metrics/stats"
    "Build VT with performance metrics/stats enabled at runtime by default"
    OFF vt_feature_cmake_diagnostics_runtime
)
define_option(vt_diagnostics_enabled "diagnostics (performance stats)"
    "Build VT with performance metrics/stats" OFF vt_feature_cmake_diagnostics
)
define_option(vt_libfort_enabled "libfort" "Build VT with fort library enabled"
    ${vt_diagnostics_enabled} vt_feature_cmake_libfort
)

if (NOT vt_diagnostics_enabled)
    set(vt_feature_cmake_diagnostics_runtime "0")
endif()

#####################################################
define_option(vt_production_build_enabled "production build"
    "Build VT with assertions and debug prints disabled" OFF vt_feature_cmake_production_build
)

set (vt_feature_cmake_no_feature "0")
set (vt_feature_cmake_mpi_rdma "0")
set (vt_feature_cmake_print_term_msgs "0")
set (vt_feature_cmake_no_pool_alloc_env "0")

define_option(vt_ci_build "CI build"
    "Build VT with CI mode on" OFF vt_feature_cmake_ci_build
)

define_option(vt_no_color_enabled "--vt_no_color set to true by default"
    "Build VT with option --vt_no_color set to true by default" OFF empty_feature
)

define_option(vt_code_coverage "code coverage" "Enable coverage reporting" OFF empty_feature)
define_option(vt_gold_linker_enabled "`gold' linker" "Build VT using the `gold' linker" ON empty_feature)
define_option(vt_unity_build_enabled "unity build" "Build VT with Unity/Jumbo mode enabled" OFF empty_feature)
define_option(vt_mimalloc_enabled "mimalloc" "Build VT with mimalloc" OFF empty_feature)
define_option(vt_mimalloc_static "mimalloc using static linking" "Build VT with mimalloc using static linking"
    ON empty_feature
)
define_option(vt_asan_enabled "address sanitizer" "Build VT with address sanitizer" OFF empty_feature)
define_option(vt_ubsan_enabled "undefined behavior sanitizer" "Build VT with undefined behavior sanitizer"
    OFF empty_feature
)
define_option(vt_werror_enabled "-Werror" "Build VT with -Werror enabled" OFF empty_feature)
define_option(vt_build_tests "tests" "Build VT tests" ON empty_feature)
define_option(vt_build_tools "tools" "Build VT tools" ON empty_feature)
define_option(vt_build_examples "examples" "Build VT examples" ON empty_feature)

list(POP_BACK CMAKE_MESSAGE_INDENT)

##########################################################################################################
