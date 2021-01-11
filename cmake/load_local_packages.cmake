
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)


# require directories for these packages
require_pkg_directory(detector "VT detector library")
# find these required packages locally
find_package_local(detector "${detector_DIR}" detector)

# require directories for these packages
require_pkg_directory(checkpoint "VT checkpoint library")
# find these required packages locally
find_package_local(checkpoint "${checkpoint_DIR}" checkpoint)

set(CHECKPOINT_LIBRARY vt::lib::checkpoint)
