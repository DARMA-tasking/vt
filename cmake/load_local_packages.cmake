
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

if (EXISTS "${PROJECT_LIB_DIR}/detector")
  add_subdirectory(${PROJECT_LIB_DIR}/detector)
  set(vt_find_detector_dep 0)
else()
  # require directories for these packages
  require_pkg_directory(detector "VT detector library")
  # find these required packages locally
  find_package_local(detector "${detector_DIR}" detector)
  set(vt_find_detector_dep 1)
endif()

if (EXISTS "${PROJECT_LIB_DIR}/checkpoint")
  add_subdirectory(${PROJECT_LIB_DIR}/checkpoint)
  set(vt_find_checkpoint_dep 0)
else()
  # require directories for these packages
  require_pkg_directory(checkpoint "VT checkpoint library")
  # find these required packages locally
  find_package_local(checkpoint "${checkpoint_DIR}" checkpoint)
  set(vt_find_checkpoint_dep 1)
endif()

set(CHECKPOINT_LIBRARY vt::lib::checkpoint)
