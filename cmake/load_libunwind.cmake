set(vt_feature_cmake_libunwind "0")

cmake_policy(PUSH)

cmake_policy(SET CMP0144 NEW)

if(NOT DEFINED LIBUNWIND_ROOT)
    set(LIBUNWIND_ROOT "/usr")
endif()

find_package(libunwind)

cmake_policy(POP)

if(LIBUNWIND_FOUND)
    set(vt_feature_cmake_libunwind "1")
endif()
