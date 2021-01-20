
#
#  Load and discover threading settings
#

include(cmake/threading_config.cmake)

# Threading build configuration
option(USE_STD_THREAD "whether to force use of std::thread for threading" OFF)
option(USE_OPENMP "whether to force use of OpenMP for threading" OFF)

if (USE_STD_THREAD)
  message(
    STATUS
    "Using std::thread for worker threading"
  )
  config_for_std_thread()
elseif(USE_OPENMP)
  message(
    STATUS
    "Using OpenMP for worker threading"
  )
  find_package(OpenMP)
  config_for_openmp()
  if (NOT OpenMP_FOUND)
    message(
      FATAL_ERROR
      "requested OpenMP with -DUSE_OPENMP=On, but cannot find "
      "valid OpenMP in compiler"
    )
  endif()
else()
  message(
    STATUS
    "No threading configuration provided"
  )
  config_no_threading()
endif()
