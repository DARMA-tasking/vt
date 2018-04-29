

# Local packages that VT depends on (meld/detector/checkpoint)
include(cmake/load_local_packages.cmake)

# MPI package
include(cmake/load_mpi_package.cmake)
# ZLIB package
include(cmake/load_zlib_package.cmake)
# FMT package
include(cmake/load_fmt_package.cmake)
# Google test package
include(cmake/load_gtest_package.cmake)

# Discover and load threading configuration
include(cmake/load_threading_package.cmake)
