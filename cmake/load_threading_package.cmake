#
#  Load and discover threading settings
#

# Threading build configuration
option(vt_fcontext_enabled "Build VT with fcontext (ULT) enabled" OFF)

if(vt_fcontext_enabled)
  message(
    STATUS
    "Using fcontext for worker threading"
  )
else()
  message(
    STATUS
    "Threading disabled"
  )
endif()
