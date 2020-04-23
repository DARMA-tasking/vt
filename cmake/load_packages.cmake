get_directory_property(projHasParent PARENT_DIRECTORY)

# Local packages that VT depends on (detector/checkpoint)
include(cmake/load_local_packages.cmake)

# MPI package
include(cmake/load_mpi_package.cmake)

# ZLIB package
include(cmake/load_zlib_package.cmake)

# Discover and load threading configuration
include(cmake/load_threading_package.cmake)

# Perl is used to build the PMPI wrappers
find_package(Perl)

# Doxygen package
include(cmake/load_doxygen.cmake)

# Doxygen package
include(cmake/load_zoltan_package.cmake)

# Tests
include(cmake/test_vt.cmake)
