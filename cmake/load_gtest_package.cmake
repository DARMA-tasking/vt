
#
#  Load the google test and ctest package (optional)
#

# Google test cmake option and package setup
option(
  enable_gtest "Enable google test framework: -Dgtest_DIR= to specify path" ON
)

if (${enable_gtest})
  if (${gtest_DIR})
    set(GTEST_ROOT "${gtest_DIR}" CACHE PATH "Path to googletest")
  endif()

  # Instead of explicitly including (include(FindGTest)), call find_package
  # which automatically searches the module include path. This is the preferred
  # mechanism for utilizing a package script.
  if(NOT projHasParent)
    find_package(GTest REQUIRED)

    if(GTEST_FOUND)
      set(GTEST_DIR "${GTEST_ROOT}")
    else()
      message(
        FATAL_ERROR
        "Gtest not found, not building tests. "
        "Please specify valid directory with -Dgtest_DIR="
      )
    endif()
  else()
    include(cmake-modules/GoogleTest.cmake)
    set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  endif()

  set(VT_HAS_GTEST TRUE)
else()
  message(
    STATUS
    "Gtest is disabled; not building tests. To enable set -Denable_gtest=true."
    )
  set(VT_HAS_GTEST FALSE)
endif()

include (CTest)
enable_testing()

include(cmake/test_vt.cmake)
