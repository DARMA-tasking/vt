if (NOT vt_lb_enabled)
  return()
endif()

set(
  VT_LB_REPOSITORY_DIR "${PROJECT_LIB_DIR}/LB"
  CACHE PATH "Path to a checkout of DARMA-tasking/LB"
)
set(
  VT_COMM_REPOSITORY_DIR "${PROJECT_LIB_DIR}/comm"
  CACHE PATH "Path to a checkout of DARMA-tasking/comm"
)

set(_vt_lb_header "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/temperedlb.h")
set(_vt_comm_header "${VT_COMM_REPOSITORY_DIR}/src/comm/comm/vt/comm_vt.h")

if (NOT EXISTS "${_vt_lb_header}")
  message(FATAL_ERROR
    "VT load balancing requires a DARMA-tasking/LB checkout. "
    "Clone it into '${PROJECT_LIB_DIR}/LB' or set VT_LB_REPOSITORY_DIR."
  )
endif()
if (NOT EXISTS "${_vt_comm_header}")
  message(FATAL_ERROR
    "VT load balancing requires a DARMA-tasking/comm checkout. "
    "Clone it into '${PROJECT_LIB_DIR}/comm' or set VT_COMM_REPOSITORY_DIR."
  )
endif()

# Only the VT communication backend and the algorithm sources are compiled into
# VT. This deliberately avoids nesting either repository's top-level CMake
# project, which would introduce a package dependency cycle (comm -> VT -> LB).
set(VT_EXTERNAL_LB_SOURCE_FILES
  "${VT_COMM_REPOSITORY_DIR}/lib/fmt/src/format.cc"
  "${VT_COMM_REPOSITORY_DIR}/src/comm/comm/vt/comm_vt.cc"
  "${VT_COMM_REPOSITORY_DIR}/src/comm/util/logging.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/baselb/baselb.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/cluster_summarizer.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/temperedlb.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/transfer_util.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/work_model.cc"
)

set(vt_backend_feature_enabled 1)
configure_file(
  "${VT_COMM_REPOSITORY_DIR}/src/comm/config/cmake_config.h.in"
  "${PROJECT_BIN_DIR}/external/comm/config/cmake_config.h"
  @ONLY
)

set(VT_EXTERNAL_LB_INCLUDE_DIRS
  "${VT_COMM_REPOSITORY_DIR}/lib/fmt/include"
  "${VT_COMM_REPOSITORY_DIR}/src"
  "${VT_LB_REPOSITORY_DIR}/src"
  "${PROJECT_BIN_DIR}/external"
)
