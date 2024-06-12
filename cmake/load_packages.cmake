get_directory_property(projHasParent PARENT_DIRECTORY)

# MPI package
include(cmake/load_mpi_package.cmake)

# ZLIB package
find_package(ZLIB REQUIRED)

# Perl is used to build the PMPI wrappers
find_package(Perl)

# Doxygen package
include(cmake/load_doxygen.cmake)

# Optionally link with libunwind
include(cmake/load_libunwind.cmake)

# Optionally link with Zoltan
include(cmake/load_zoltan_package.cmake)

# Link with PAPI
include(cmake/load_papi.cmake)

# Tests
include(cmake/test_vt.cmake)
