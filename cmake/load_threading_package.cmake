#
#  Load and discover threading settings
#

# Enable fcontext threading
function(config_for_fcontext)
  set(vt_fcontext_enabled "1" PARENT_SCOPE)
endfunction(config_for_fcontext)

# Disable fcontext threading
function(config_no_threading)
  set(vt_fcontext_enabled "0" PARENT_SCOPE)
endfunction(config_no_threading)

# Threading build configuration
option(vt_fcontext_enabled "Build VT with fcontext (ULT) enabled" OFF)

if(vt_fcontext_enabled)
  message(
    STATUS
    "Using fcontext for worker threading"
  )
  config_for_fcontext()
else()
  message(
    STATUS
    "Threading disabled"
  )
  config_no_threading()
endif()
