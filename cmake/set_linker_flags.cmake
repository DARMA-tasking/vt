include(CheckLinkerFlag)

check_linker_flag(-rdynamic LINKER_SUPPORTS_RDYNAMIC)

if (LINKER_SUPPORTS_RDYNAMIC)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    target_link_options(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE -rdynamic)
  else()
    target_link_libraries(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE -rdynamic)
  endif()
endif()
