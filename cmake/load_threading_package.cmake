
#
#  Load and discover threading settings
#

include(cmake/threading_config.cmake)

# Threading build configuration
# VT_THREADING_BACKEND can be "openmp" "stdthread" or "none
option(USE_STD_THREAD "whether to force use of std::thread for threading" OFF)
option(USE_OPENMP "whether to force use of OpenMP for threading" OFF)

if (NOT VT_THREADING_BACKEND)
  set(VT_THREADING_BACKEND "none")
endif()

# OpenMP support
if (VT_THREADING_BACKEND STREQUAL "stdthread")
  message("Using std::thread for worker threading")
elseif(VT_THREADING_BACKEND STREQUAL "openmp")
  config_for_openmp()
  find_package(OpenMP REQUIRED)
elseif(VT_THREADING_BACKEND STREQUAL "none")
  message("No worker threading enabled")
else()
  message(FATAL_ERROR "invalid value for VT_THREADING_BACKEND, must be one of \"openmp\", \"stdthread\", or \"none\" (default \"none\")")
endif()

set(DEFAULT_THREADING ${VT_THREADING_BACKEND})
