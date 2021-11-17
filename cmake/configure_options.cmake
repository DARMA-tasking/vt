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

list(APPEND CMAKE_MESSAGE_INDENT "Building VT with ")

option(vt_lb_enabled "Build VT with load balancing enabled" ON)
option(vt_trace_enabled "Build VT with trace enabled" OFF)
option(vt_priorities_enabled "Build VT with message priorities enabled" ON)
option(
        vt_test_trace_runtime_enabled
        "Build VT with runtime tracing enabled (for testing)" OFF
)
option(vt_pool_enabled "Build VT with memory pool" ON)

set(
        vt_priority_bits_per_level 3 CACHE
        STRING "Number of bits to use per VT priority level"
)

if (${vt_test_trace_runtime_enabled})
    message(STATUS "runtime tracing enabled (for testing)")
    set(vt_feature_cmake_test_trace_on "1")
else()
    set(vt_feature_cmake_test_trace_on "0")
endif()

if (${vt_lb_enabled})
    message(STATUS "load balancing enabled")
    set(vt_feature_cmake_lblite "1")
else()
    message(STATUS "load balancing disabled")
    set(vt_feature_cmake_lblite "0")
endif()

if (${vt_trace_enabled})
    message(STATUS "tracing enabled")
    set(vt_feature_cmake_trace_enabled "1")
else()
    message(STATUS "tracing disabled")
    set(vt_feature_cmake_trace_enabled "0")
endif()

# This will be set to 1 during generation of cmake config for vt-trace target
set(vt_feature_cmake_trace_only "0")

if (${vt_priorities_enabled})
    message(STATUS "priorities enabled")
    message(
            STATUS
            "priority bits per level: ${vt_priority_bits_per_level}"
    )
    set(vt_feature_cmake_priorities "1")
else()
    message(STATUS "priorities disabled")
    set(vt_feature_cmake_priorities "0")
endif()

set(vt_feature_cmake_priority_bits_level "${vt_priority_bits_per_level}")

if (${vt_bit_check_overflow})
    message(STATUS "bit check overflow")
    set(vt_feature_cmake_bit_check_overflow "1")
else()
    set(vt_feature_cmake_bit_check_overflow "0")
endif()

if (${vt_fcontext_enabled})
    message(STATUS "fcontext (ULT) enabled")
    set(vt_feature_cmake_fcontext "1")
else()
    message(STATUS "fcontext (ULT) disabled")
    set(vt_feature_cmake_fcontext "0")
endif()

if (${vt_mimalloc_enabled})
    message(STATUS "mimalloc enabled")
    set(vt_feature_cmake_mimalloc "1")
else()
    message(STATUS "mimalloc disabled")
    set(vt_feature_cmake_mimalloc "0")
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_CMAKE_BUILD_TYPE)
if (LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "debug")
    option(vt_mpi_guards "Build VT with poison MPI calls: code invoked from VT callbacks cannot invoke MPI functions" ON)
else()
    option(vt_mpi_guards "Build VT with poison MPI calls: code invoked from VT callbacks cannot invoke MPI functions" OFF)
endif()

if ((vt_mpi_guards OR vt_trace_only) AND PERL_FOUND)
    message(STATUS "user MPI prevention guards enabled")
    set(vt_feature_cmake_mpi_access_guards "1")
elseif ((vt_mpi_guards OR vt_trace_only) AND NOT PERL_FOUND)
    # No perl? Can't generate wrapper source file.
    message(STATUS "user MPI prevention guards disabled (requested, but perl not found)")
    set(vt_feature_cmake_mpi_access_guards "0")
else()
    message(STATUS "user MPI prevention guards disabled")
    set(vt_feature_cmake_mpi_access_guards "0")
endif()

option(vt_zoltan_enabled "Build VT with Zoltan" OFF)
if (vt_zoltan_enabled AND vt_zoltan_found)
    message(STATUS "zoltan enabled")
    set(vt_feature_cmake_zoltan "1")
elseif (vt_zoltan_enabled AND NOT vt_zoltan_found)
    message(STATUS "zoltan disabled (requested, but Zoltan not found)")
    set(vt_feature_cmake_zoltan "0")
else()
    message(STATUS "zoltan disabled")
    set(vt_feature_cmake_zoltan "0")
endif()

if (LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "release")
    option(vt_debug_verbose "Build VT with verbose debug printing enabled" OFF)
else()
    option(vt_debug_verbose "Build VT with verbose debug printing enabled" ON)
endif()

if(vt_debug_verbose)
    message(STATUS "verbose debug printing enabled")
    set(vt_feature_cmake_debug_verbose "1")
else()
    message(STATUS "verbose debug printing disabled")
    set(vt_feature_cmake_debug_verbose "0")
endif()

option(
        vt_diagnostics_runtime_enabled
        "Build VT with performance metrics/stats enabled at runtime by default" OFF
)
if (vt_diagnostics_enabled)
    message(STATUS "diagnostics (performance stats) enabled")
    set(vt_feature_cmake_diagnostics "1")

    if (vt_diagnostics_runtime_enabled)
        message(
                STATUS
                "diagnostics (performance stats) enabled at runtime by default"
        )
        set(vt_feature_cmake_diagnostics_runtime "1")
    else()
        message(
                STATUS
                "diagnostics (performance stats) disabled at runtime by default"
        )
        set(vt_feature_cmake_diagnostics_runtime "0")
    endif()

else()
    message(STATUS "diagnostics (performance stats) disabled")
    set(vt_feature_cmake_diagnostics "0")
    set(vt_feature_cmake_diagnostics_runtime "0")
endif()

option(
        vt_production_build_enabled
        "Build VT with assertions and debug prints disabled" OFF
)
if (${vt_production_build_enabled})
    message(STATUS "assertions and debug prints disabled")
    set(vt_feature_cmake_production_build "1")
else()
    message(STATUS "assertions and debug prints enabled")
    set(vt_feature_cmake_production_build "0")
endif()

if (vt_libfort_enabled)
    set(vt_feature_cmake_libfort "1")
else()
    set(vt_feature_cmake_libfort "0")
endif()

set(vt_feature_cmake_no_feature "0")

set (vt_feature_cmake_mpi_rdma "0")
set (vt_feature_cmake_print_term_msgs "0")
set (vt_feature_cmake_no_pool_alloc_env "0")

if (${vt_pool_enabled})
    message(STATUS "memory pool enabled")
    set (vt_feature_cmake_memory_pool "1")
else()
    message(STATUS "memory pool disabled")
    set (vt_feature_cmake_memory_pool "0")
endif()

option(vt_ci_build "Build VT with CI mode on" OFF)
if(${vt_ci_build})
    set(vt_feature_cmake_ci_build "1")
else()
    set(vt_feature_cmake_ci_build "0")
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)

message(STATUS "CI_BUILD = ${vt_feature_cmake_ci_build}")
