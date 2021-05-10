include(CheckLinkerFlag)

check_linker_flag(-rdynamic LINKER_SUPPORTS_RDYNAMIC)

if (LINKER_SUPPORTS_RDYNAMIC)
  target_link_options(${VIRTUAL_TRANSPORT_LIBRARY} INTERFACE -rdynamic)
endif()
