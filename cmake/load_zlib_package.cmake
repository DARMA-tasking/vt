
#
#  Load Zlib settings (required)
#

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message("zlib is required for tracing")
endif(ZLIB_FOUND)
