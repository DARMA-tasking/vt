
#
#  Discover and load Backtrace (required)
#

find_package(Backtrace REQUIRED)
if(NOT Backtrace_FOUND)
  message(FATAL_ERROR "Backtrace not found")
endif()
