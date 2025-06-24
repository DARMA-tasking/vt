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

function "vt_build_shared_libs" {
  params = [item]
  result = lookup(item, "vt_build_shared_libs", "")
}

function "vt_build_trace_only" {
  params = [item]
  result = lookup(item, "vt_build_trace_only", "1")
}

function "vt_ci_build" {
  params = [item]
  result = lookup(item, "vt_ci_build", "1")
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

function "vt_inclusion" {
  params = [item]
  result = lookup(item, "vt_inclusion", "TPL")
}

function "vt_kokkos" {
  params = [item]
  result = lookup(item, "vt_kokkos", "")
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


target "vt-build" {
  target = "build"
  context = "."
  dockerfile = "ci/docker/vt.dockerfile"
  output = [
    {
      type = "local"
      dest = "docker-output"
    }
  ]
  platforms = [
    "linux/amd64",
    # "linux/arm64"
  ]
  ulimits = [
    "core=0"
  ]
  # FIXME: verify that caching works as intended
  # cache-from = [
  #   {
  #     type = "local",
  #     src = "~/ccache"
  #   }
  # ]
  # cache-to = [
  #   {
  #     type = "local",
  #     dest = "~/ccache"
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
    BUILD_SHARED_LIBS              = vt_build_shared_libs(item)
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
    VT_INCLUSION_TYPE              = vt_inclusion(item)
    VT_KOKKOS_ENABLED              = vt_kokkos(item)
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
        image = "amd64-ubuntu-22.04-gcc-12-cpp"
        vt_debug_verbose = 1
        vt_kokkos = 1
      },
    ]
  }
}
