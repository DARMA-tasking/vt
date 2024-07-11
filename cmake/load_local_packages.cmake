
#
#  Require the local packages that VT depends on
#

include(cmake/local_package.cmake)

if (EXISTS "${PROJECT_LIB_DIR}/checkpoint")
  add_subdirectory(${PROJECT_LIB_DIR}/checkpoint)
elseif(EXISTS "${PROJECT_LIB_DIR}/magistrate")
  add_subdirectory(${PROJECT_LIB_DIR}/magistrate)
else()
  # find these required packages locally
  find_package_local(magistrate)
  endif()

set(MAGISTRATE_LIBRARY vt::lib::magistrate)

if (EXISTS "${PROJECT_LIB_DIR}/vt-tv" AND vt_tv_enabled)
  set(vt_tv_python_bindings_enabled OFF)
  add_subdirectory(${PROJECT_LIB_DIR}/vt-tv)
  set(TV_LIBRARY vt::lib::vt-tv)
endif()
