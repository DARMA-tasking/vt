set(vt_zoltan_found "0")

if(vt_zoltan_enabled)
  find_package(Zoltan REQUIRED)
  set(ZOLTAN_DEPENDENCY "find_dependency(Zoltan REQUIRED)" CACHE STRING "Rule for Zoltan dependency used in vtConfig" FORCE)
  set(vt_zoltan_found "1")
endif()
