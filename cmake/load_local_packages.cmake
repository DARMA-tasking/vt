
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

if (EXISTS "${PROJECT_LIB_DIR}/detector")
  add_subdirectory(${PROJECT_LIB_DIR}/detector)
else()
  # require directories for these packages
  require_pkg_directory(detector "VT detector library")
  # find these required packages locally
  find_package_local(detector "${detector_DIR}" detector)
endif()

if (EXISTS "${PROJECT_LIB_DIR}/checkpoint")
  add_subdirectory(${PROJECT_LIB_DIR}/checkpoint)
else()
  # require directories for these packages
  require_pkg_directory(checkpoint "VT checkpoint library")
  # find these required packages locally
  find_package_local(checkpoint "${checkpoint_DIR}" checkpoint)
endif()

set(CHECKPOINT_LIBRARY vt::lib::checkpoint)
