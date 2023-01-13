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
  set(vt_fcontext_enabled "1")
else()
  message(
    STATUS
    "Threading disabled"
  )
  set(vt_fcontext_enabled "0")
endif()
