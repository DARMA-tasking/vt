

set(vt_zoltan_found "0")

# optional directory for this package
optional_pkg_directory(zoltan "Zoltan library" 1)
# find the optional packages locally if identified
if (${zoltan_DIR_FOUND})
  find_package_local(zoltan "${zoltan_DIR}" Zoltan)
  if(NOT projHasParent)
    if (NOT ${zoltan_FOUND})
      message(FATAL_ERROR "Zoltan library not found")
    endif()
  endif()
  set(vt_zoltan_found "1")
endif()
