
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

if (EXISTS "${PROJECT_LIB_DIR}/checkpoint")
  add_subdirectory(${PROJECT_LIB_DIR}/checkpoint)
else()
  # find these required packages locally
  find_package_local(checkpoint checkpoint)
  endif()

set(CHECKPOINT_LIBRARY vt::lib::checkpoint)
