set(vt_has_libunwind 0)

find_package(libunwind)

if(libunwind_FOUND)
    set(vt_has_libunwind 1)
endif()
