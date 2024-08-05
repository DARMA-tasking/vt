set(vt_feature_cmake_libunwind "0")

if(NOT DEFINED LIBUNWIND_ROOT)
    set(LIBUNWIND_ROOT "/usr")
endif()

find_package(libunwind)

if(LIBUNWIND_FOUND)
    set(vt_feature_cmake_libunwind "1")
endif()
