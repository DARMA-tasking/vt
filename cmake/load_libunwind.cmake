set(vt_feature_cmake_libunwind "0")

find_package(libunwind)

if(LIBUNWIND_FOUND)
    set(vt_feature_cmake_libunwind "1")
endif()
