
#
#  Load the FMT package
#
#    @todo: currently required, but eventually should be optional
#

find_package_local(fmt "${fmt_DIR}" fmt "lib/cmake/fmt")
if(NOT fmt_FOUND)
  message(
    FATAL_ERROR
    "VT requires the fmt library. Please specify valid installation with "
    "-Dfmt_DIR= or install by downloading from https://github.com/fmtlib/fmt"
  )
endif()
