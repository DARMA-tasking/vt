variable "COMPILER_TYPE" {
  default = "clang"
}

variable "HOST_COMPILER" {
  default = "clang-14"
}

variable "COMPILER" {
  default = "clang-14"
}

variable "REPO" {
  default = "lifflander1/vt"
}

variable "ARCH" {
  default = "amd64"
}

variable "DISTRO" {
  default = "ubuntu"

  validation {
    condition = DISTRO == "ubuntu" || DISTRO == "alpine"
    error_message = "Supported configurations are ubuntu and alpine"
  }
}

variable "DISTRO_VERSION" {
  default = "22.04"
}

variable "VT_LB" {
  default = "1"
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
  name = "vt-build-${item.arch}-${item.distro}-${replace(item.distro_version, ".", "-")}-${item.compiler}-cpp"
  inherits = ["vt-build"]
  # tags = ["${REPO}:${ARCH}-ubuntu-${UBUNTU}-${HOST_COMPILER}-${COMPILER}-cpp"]

  args = {
    ARCH = "${item.arch}"
    DISTRO = "${item.distro}"
    DISTRO_VERSION = "${item.distro_version}"
    COMPILER = "${item.compiler}"
    VT_LB_ENABLED = "${item.vt_lb}"
    VT_BUILD_TESTS = "0"
    VT_BUILD_EXAMPLES = "0"
  }

  matrix = {
    item = [
      {
        arch = "amd64"
        distro = "ubuntu"
        distro_version = "22.04"
        compiler = "clang-13"
        mpi = "mpich"
        vt_lb = "1"
      },
      {
        arch = "amd64"
        distro = "ubuntu"
        distro_version = "22.04"
        compiler = "clang-14"
        mpi = "mpich"
        vt_lb = "0"
      }

    ]
  }
}

target "vt-test" {
  inherits = ["vt-build"]
  target = "test"
}
