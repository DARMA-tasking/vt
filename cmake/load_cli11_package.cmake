
#
#  Load the FMT package
#
#    @todo: currently required, but eventually should be optional
#

find_package_local(CLI11 "${CLI11_DIR}" CLI11 "lib/cmake/CLI11")
if(NOT CLI11_FOUND)
  message(
    FATAL_ERROR
    "VT requires the CLI11 library. Please specify valid installation with "
    "-DCLI11_DIR= or install by downloading: https://github.com/CLIUtils/CLI11"
  )
endif()
