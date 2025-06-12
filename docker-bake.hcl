variable "REPO" {
  default = "lifflander1/vt"
}

function "arch" {
  params = [item]
  result = lookup(item, "arch", "amd64")
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
  name = "vt-build-${replace(item.image, ".", "-")}"
  inherits = ["vt-build"]
  tags = ["${REPO}:vt-${item.image}"]

  args = {
    ARCH = arch(item)
    IMAGE = "wf-${item.image}"
    REPO = REPO
    VT_LB_ENABLED = item.vt_lb
    VT_BUILD_TESTS = "0"
    VT_BUILD_EXAMPLES = "0"
  }

  matrix = {
    item = [
      {
        image = "amd64-ubuntu-22.04-clang-13-cpp"
        vt_lb = "1"
      },
      {
        image = "amd64-ubuntu-22.04-clang-14-cpp"
        vt_lb = "0"
      }

    ]
  }
}

target "vt-test" {
  inherits = ["vt-build"]
  target = "test"
}
