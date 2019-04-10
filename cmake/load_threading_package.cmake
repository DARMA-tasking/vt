
#
#  Load and discover threading settings
#

include(cmake/threading_config.cmake)

# Threading build configuration
option(USE_STD_THREAD "whether to force use of std::thread for threading" OFF)
option(USE_OPENMP "whether to force use of OpenMP for threading" OFF)

find_package(OpenMP)

# OpenMP support
if (USE_STD_THREAD)
  message("Using std::thread for worker threading")
  config_for_std_thread()
elseif(USE_OPENMP)
  config_for_openmp()
  if (NOT OpenMP_FOUND)
    message(
      FATAL_ERROR
      "requested OpenMP with -DUSE_OPENMP=On, but cannot find "
      "valid OpenMP in compiler"
    )
  endif()
endif()
