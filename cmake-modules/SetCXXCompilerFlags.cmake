function(set_darma_compiler_flags vt_target)
  if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    # 4.9.3 complains about std::min not being constexpr
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5))
      message("${PROJECT_NAME} currently requires g++ 5 or greater.  If you need it to work with 4.9, please complain.")
    endif ()
  elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    list(APPEND TARGET_PUBLIC_CXX_FLAGS -ftemplate-depth=900)
    if (APPLE)
      list(APPEND TARGET_PUBLIC_CXX_FLAGS -stdlib=libc++)
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 2021.3.0)
    list(APPEND TARGET_PRIVATE_CXX_FLAGS -fhonor-infinites -fhonor-nans)
  elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    # Intel classic (icpc)
    # Set as PUBLIC because we want to propagate it to "derived" targets
    # gtest-main links with gtest, while VT's tests link with base VT
    list(APPEND TARGET_PUBLIC_CXX_FLAGS -diag-disable=10441)
    list(APPEND TARGET_PUBLIC_LINK_FLAGS -diag-disable=10441)
  elseif (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    message(FATAL_ERROR "Your C++ compiler may not support C++14.")
  endif ()

  if ("${vt_target}" STREQUAL "${VIRTUAL_TRANSPORT_LIBRARY}")
    if (vt_asan_enabled)
      list(APPEND TARGET_PUBLIC_CXX_FLAGS -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()

    if (vt_ubsan_enabled)
      add_definitions(-DVT_UBSAN_ENABLED)
      list(APPEND TARGET_PUBLIC_CXX_FLAGS -fsanitize=undefined -fno-omit-frame-pointer)
    endif()
  endif()

  message(DEBUG "Target ${vt_target} public compile options: ${TARGET_PUBLIC_CXX_FLAGS} \n public link options: ${TARGET_PUBLIC_LINK_FLAGS}")
  target_compile_options(${vt_target} PUBLIC ${TARGET_PUBLIC_CXX_FLAGS})
  target_link_options(${vt_target} PUBLIC ${TARGET_PUBLIC_LINK_FLAGS})

  message(DEBUG "Target ${vt_target} private compile options: ${TARGET_PRIVATE_CXX_FLAGS}")
  target_compile_options(${vt_target} PRIVATE ${TARGET_PRIVATE_CXX_FLAGS})

endfunction()
