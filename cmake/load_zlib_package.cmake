
#
#  Load Zlib settings (required)
#

find_package(ZLIB REQUIRED)
if(NOT ZLIB_FOUND)
  message("zlib is required for tracing")
endif(NOT ZLIB_FOUND)
