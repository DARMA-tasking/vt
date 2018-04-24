
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

# require directories for these packages
require_pkg_directory(darma_meld     "DARMA meld")
require_pkg_directory(darma_detector "DARMA detector")
# find these required packages locally
find_package_local(darma_meld     "${darma_meld_DIR}/cmake")
find_package_local(darma_detector "${darma_detector_DIR}/cmake")
# optional directory for this package
optional_pkg_directory(darma_checkpoint "Serialization/Checkpoint")
# find the optional packages locally if identified
if (${darma_checkpoint_DIR_FOUND})
  find_package_local(darma_checkpoint "${darma_checkpoint_DIR}/cmake")
  if (${darma_checkpoint_FOUND})
    set(VT_HAS_SERIALIZATION_LIBRARY 1)
    set(CHECKPOINT_LIBRARY darma_checkpoint)
  else()
    message(FATAL_ERROR "Serialization/checkpoint library not found")
  endif()
endif()
