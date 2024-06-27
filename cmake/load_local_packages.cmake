
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

if (EXISTS "${PROJECT_LIB_DIR}/checkpoint")
  add_subdirectory(${PROJECT_LIB_DIR}/checkpoint)
else()
  # find these required packages locally
  find_package_local(magistrate)
  endif()

set(MAGISTRATE_LIBRARY vt::lib::magistrate)
