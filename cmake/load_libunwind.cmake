set(vt_feature_cmake_libunwind "0")

if(NOT DEFINED libunwind_ROOT_DIR)
    set(libunwind_ROOT_DIR "/usr")
endif()

find_package(libunwind)

if(LIBUNWIND_FOUND)
    set(vt_feature_cmake_libunwind "1")
endif()
