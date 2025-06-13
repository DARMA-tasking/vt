variable "REPO" {
  default = "lifflander1/vt"
}

function "arch" {
  params = [item]
  result = lookup(item, "arch", "amd64")
}

function "vt_asan" {
  params = [item]
  result = lookup(item, "vt_asan", "")
}

function "vt_build_trace_only" {
  params = [item]
  result = lookup(item, "vt_build_trace_only", "")
}

function "vt_ci_build" {
  params = [item]
  result = lookup(item, "vt_ci_build", "")
}

function "vt_ci_test_lb_schema" {
  params = [item]
  result = lookup(item, "vt_ci_test_lb_schema", "")
}

function "vt_code_coverage" {
  params = [item]
  result = lookup(item, "vt_code_coverage", "")
}

function "vt_debug_verbose" {
  params = [item]
  result = lookup(item, "vt_debug_verbose", "")
}

function "vt_diagnostics" {
  params = [item]
  result = lookup(item, "vt_diagnostics", "")
}

function "vt_diagnostics_runtime" {
  params = [item]
  result = lookup(item, "vt_diagnostics_runtime", "")
}

function "vt_doxygen" {
  params = [item]
  result = lookup(item, "vt_doxygen", "")
}

function "vt_extended_tests" {
  params = [item]
  result = lookup(item, "vt_extended_tests", "")
}

function "vt_external_fmt" {
  params = [item]
  result = lookup(item, "vt_external_fmt", "")
}

function "vt_fcontext" {
  params = [item]
  result = lookup(item, "vt_fcontext", "")
}

function "vt_lb" {
  params = [item]
  result = lookup(item, "vt_lb", "")
}

function "vt_mimalloc" {
  params = [item]
  result = lookup(item, "vt_mimalloc", "")
}

function "vt_mpi_guard" {
  params = [item]
  result = lookup(item, "vt_mpi_guard", "")
}

function "vt_no_color" {
  params = [item]
  result = lookup(item, "vt_no_color", "")
}

function "vt_perf" {
  params = [item]
  result = lookup(item, "vt_perf", "")
}

function "vt_pool" {
  params = [item]
  result = lookup(item, "vt_pool", "")
}

function "vt_production_build" {
  params = [item]
  result = lookup(item, "vt_production_build", "")
}

function "vt_rdma_tests" {
  params = [item]
  result = lookup(item, "vt_rdma_tests", "")
}

function "vt_tests_num_nodes" {
  params = [item]
  result = lookup(item, "vt_tests_num_nodes", "")
}

function "vt_trace" {
  params = [item]
  result = lookup(item, "vt_trace", "")
}

function "vt_trace_runtime" {
  params = [item]
  result = lookup(item, "vt_trace_runtime", "")
}

function "vt_tv" {
  params = [item]
  result = lookup(item, "vt_tv", "")
}

function "vt_ubsan" {
  params = [item]
  result = lookup(item, "vt_ubsan", "")
}

function "vt_unity_build" {
  params = [item]
  result = lookup(item, "vt_unity_build", "")
}

function "vt_werror" {
  params = [item]
  result = lookup(item, "vt_werror", "")
}

function "vt_zoltan" {
  params = [item]
  result = lookup(item, "vt_zoltan", "")
}

variable "CCACHE_DIR" {
  default = "/ccache"
}

target "vt-build" {
  target = "build"
  context = "."
  dockerfile = "ci/docker/vt.dockerfile"
  platforms = [
    "linux/amd64",
    # "linux/arm64"
  ]
  ulimits = [
    "core=0"
  ]
  # FIXME: verify that caching works as intended
  # cache_from = [
  #   {
  #     type = "local",
  #     src = "/ccache"
  #   }
  # ]
  # cache_to = [
  #   {
  #     type = "local",
  #     dest = "/ccache"
  #   }
  # ]
}

target "vt-build-all" {
  name = "vt-build-${replace(item.image, ".", "-")}"
  inherits = ["vt-build"]
  tags = ["${REPO}:vt-${item.image}"]

  args = {
    ARCH = arch(item)
    IMAGE = "wf-${item.image}"
    REPO = REPO
    VT_ASAN_ENABLED                = vt_asan(item)
    VT_BUILD_TRACE_ONLY            = vt_build_trace_only(item)
    VT_CI_BUILD                    = vt_ci_build(item)
    VT_CI_TEST_LB_SCHEMA           = vt_ci_test_lb_schema(item)
    VT_CODE_COVERAGE               = vt_code_coverage(item)
    VT_DEBUG_VERBOSE               = vt_debug_verbose(item)
    VT_DIAGNOSTICS_ENABLED         = vt_diagnostics(item)
    VT_DIAGNOSTICS_RUNTIME_ENABLED = vt_diagnostics_runtime(item)
    VT_DOXYGEN_ENABLED             = vt_doxygen(item)
    VT_EXTENDED_TESTS_ENABLED      = vt_extended_tests(item)
    VT_EXTERNAL_FMT                = vt_external_fmt(item)
    VT_FCONTEXT_ENABLED            = vt_fcontext(item)
    VT_LB_ENABLED                  = vt_lb(item)
    VT_MIMALLOC_ENABLED            = vt_mimalloc(item)
    VT_MPI_GUARD_ENABLED           = vt_mpi_guard(item)
    VT_NO_COLOR_ENABLED            = vt_no_color(item)
    VT_PERF_ENABLED                = vt_perf(item)
    VT_POOL_ENABLED                = vt_pool(item)
    VT_PRODUCTION_BUILD_ENABLED    = vt_production_build(item)
    VT_RDMA_TESTS_ENABLED          = vt_rdma_tests(item)
    VT_TESTS_NUM_NODES             = vt_tests_num_nodes(item)
    VT_TRACE_ENABLED               = vt_trace(item)
    VT_TRACE_RUNTIME_ENABLED       = vt_trace_runtime(item)
    VT_TV_ENABLED                  = vt_tv(item)
    VT_UBSAN_ENABLED               = vt_ubsan(item)
    VT_UNITY_BUILD_ENABLED         = vt_unity_build(item)
    VT_WERROR_ENABLED              = vt_werror(item)
    VT_ZOLTAN_ENABLED              = vt_zoltan(item)
  }

  # to get the list of available images from DARMA-tasking/workflows:
  # workflows > docker buildx bake --print build-all | grep "lifflander1/vt:"
  matrix = {
    item = [
      {
        image = "amd64-alpine-3.16-clang-cpp"
        vt_production_build = 1
      },
      {
        image = "amd64-ubuntu-20.04-clang-10-cpp"
        vt_tests_num_nodes = 4
        vt_ubsan = 1
        #FIXME
        ubsan_options = "print_stacktrace=1"
      },
      {
        image = "amd64-ubuntu-20.04-clang-9-cpp"
        vt_build_shared_libs = 1
        vt_inclusion = "EXT_LIB"
        vt_werror = 0
      },
      {
        image = "amd64-ubuntu-20.04-gcc-10-cpp"
      },
      {
        image = "amd64-ubuntu-20.04-gcc-10-openmpi-cpp"
        vt_lb = 0
        vt_tests_num_nodes = 4
      },
      {
        image = "amd64-ubuntu-20.04-gcc-9-cpp"
      },
      {
        image = "amd64-ubuntu-20.04-gcc-9-cuda-11.4.3-cpp"
        vt_diagnostics = 0
        vt_extended_tests = 0
        vt_external_fmt = 1
        vt_pool = 0
        vt_tests_num_nodes = 4
        vt_trace = 1
      },
      {
        image = "amd64-ubuntu-20.04-gcc-9-cuda-12.2.0-cpp"
        vt_debug_verbose = 1
        vt_diagnostics = 0
        vt_extended_tests = 0
        vt_pool = 0
        vt_tests_num_nodes = 4
        vt_trace = 1
      },
      {
        image = "amd64-ubuntu-20.04-icpx-cpp"
        vt_debug_verbose = 1
        vt_extended_tests = 0
        vt_pool = 0
        vt_trace = 1
      },
      {
        image = "amd64-ubuntu-22.04-clang-11-cpp"
        vt_fcontext = 1
      },
      {
        image = "amd64-ubuntu-22.04-clang-12-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-13-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-clang-14-cpp"
        vt_trace = 1
        vt_debug_verbose = 1
      },
      {
        image = "amd64-ubuntu-22.04-clang-15-cpp"
      },
      {
        image = "amd64-ubuntu-22.04-gcc-11-cpp"
        vt_trace = 1
        vt_trace_rt = 1
        vt_unity_build = 0
        vt_code_coverage = 1
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-cpp"
        vt_debug_verbose = 1
        #FIXME
        vt_kokkos_enabled = 1
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-vtk-cpp"
        vt_tv_enabled = 1
        vt_trace_only = 0
      },
      {
        image = "amd64-ubuntu-22.04-gcc-12-zoltan-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-vtk-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-clang-16-zoltan-cpp"
        vt_ci_test_lb_schema = 1
        vt_trace = 1
        vt_zoltan = 1
      },
      {
        image = "amd64-ubuntu-24.04-clang-17-cpp"
        vt_perf_enabled = 1
        #FIXME
        privileged = 1
      },
      {
        image = "amd64-ubuntu-24.04-clang-18-cpp"
      },
      {
        image = "amd64-ubuntu-24.04-gcc-13-cpp"
        vt_trace = 1
        vt_pool = 0
        vt_asan = 1
        vt_unity_build = 0
        #FIXME
        lsan_options = "suppressions=/vt/tests/lsan.supp"
      },
      {
        image = "amd64-ubuntu-24.04-gcc-14-cpp"
        vt_perf_enabled = 1
        #FIXME
        privileged = 1
      },
    ]
  }
}
