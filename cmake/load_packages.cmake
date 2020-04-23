get_directory_property(projHasParent PARENT_DIRECTORY)

# Local packages that VT depends on (detector/checkpoint)
include(cmake/load_local_packages.cmake)

# MPI package
include(cmake/load_mpi_package.cmake)
# ZLIB package
include(cmake/load_zlib_package.cmake)
# Google test package
include(cmake/load_gtest_package.cmake)

# Discover and load threading configuration
include(cmake/load_threading_package.cmake)

# Doxygen package
include(cmake/load_zoltan_package.cmake)
