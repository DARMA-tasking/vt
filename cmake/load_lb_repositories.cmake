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
if (NOT EXISTS "${_vt_lb_header}")
  message(FATAL_ERROR
    "VT load balancing requires a DARMA-tasking/LB checkout. "
    "Clone it into '${PROJECT_LIB_DIR}/LB' or set VT_LB_REPOSITORY_DIR."
  )
endif()

# Only the VT communication backend and the algorithm sources are compiled into
# VT. This deliberately avoids nesting either repository's top-level CMake
# project, which would introduce a package dependency cycle (comm -> VT -> LB).
set(VT_EXTERNAL_LB_SOURCE_FILES
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/baselb/baselb.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/cluster_summarizer.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/temperedlb.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/transfer_util.cc"
  "${VT_LB_REPOSITORY_DIR}/src/vt-lb/algo/temperedlb/work_model.cc"
)
set(VT_EXTERNAL_LB_INCLUDE_DIRS
  "${VT_LB_REPOSITORY_DIR}/src"
  "${PROJECT_BIN_DIR}/external"
)

# LB master carries its VT communicator. LB develop uses DARMA-tasking/comm.
# VT already links fmt, so only expose the dependency's fmt headers here.
if (EXISTS "${VT_LB_REPOSITORY_DIR}/src/vt-lb/comm/vt/comm_vt.h")
  set(_vt_comm_source "${VT_LB_REPOSITORY_DIR}/src/vt-lb")
  set(_vt_comm_config "vt-lb/config/cmake_config.h")
  list(APPEND VT_EXTERNAL_LB_INCLUDE_DIRS "${VT_LB_REPOSITORY_DIR}/tpl/fmt/include")
else()
  set(_vt_comm_source "${VT_COMM_REPOSITORY_DIR}/src/comm")
  set(_vt_comm_config "comm/config/cmake_config.h")
  if (NOT EXISTS "${_vt_comm_source}/comm/vt/comm_vt.h")
    message(FATAL_ERROR
      "This LB checkout requires DARMA-tasking/comm. "
      "Clone it into '${PROJECT_LIB_DIR}/comm' or set VT_COMM_REPOSITORY_DIR."
    )
  endif()
  list(APPEND VT_EXTERNAL_LB_INCLUDE_DIRS
    "${VT_COMM_REPOSITORY_DIR}/src"
    "${VT_COMM_REPOSITORY_DIR}/lib/fmt/include"
  )
endif()
list(APPEND VT_EXTERNAL_LB_SOURCE_FILES
  "${_vt_comm_source}/comm/vt/comm_vt.cc"
  "${_vt_comm_source}/util/logging.cc"
)

set(vt_backend_feature_enabled 1)
configure_file(
  "${_vt_comm_source}/config/cmake_config.h.in"
  "${PROJECT_BIN_DIR}/external/${_vt_comm_config}"
  @ONLY
)
