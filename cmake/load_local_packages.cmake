
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

# require directories for these packages
require_pkg_directory(meld "VT meld library")
require_pkg_directory(detector "VT detector library")
# find these required packages locally
find_package_local(meld "${meld_DIR}" meld)
find_package_local(detector "${detector_DIR}" detector)
# optional directory for this package
optional_pkg_directory(checkpoint "Serialization/Checkpoint")
# find the optional packages locally if identified
if (${checkpoint_DIR_FOUND})
  find_package_local(checkpoint "${checkpoint_DIR}" checkpoint)
  if (${checkpoint_FOUND})
    set(VT_HAS_SERIALIZATION_LIBRARY 1)
    set(CHECKPOINT_LIBRARY vt::lib::checkpoint)
  else()
    message(FATAL_ERROR "Serialization/checkpoint library not found")
  endif()
endif()
