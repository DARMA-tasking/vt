set(vt_feature_cmake_libunwind "0")

if(NOT DEFINED libunwind_ROOT)
    set(libunwind_ROOT "/usr")
endif()

find_package(libunwind)

if(libunwind_FOUND)
    set(vt_feature_cmake_libunwind "1")
endif()
