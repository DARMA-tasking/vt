if (EXISTS "${PROJECT_LIB_DIR}/loc")
  # loc detects that it is embedded and disables its standalone comm/MPI
  # backend, examples, tests, and documentation. VT supplies LocCommunicator.
  add_subdirectory(${PROJECT_LIB_DIR}/loc)
else()
  message(
    FATAL_ERROR
    "DARMA/loc was not found. Clone https://github.com/DARMA-tasking/loc.git "
    "into ${PROJECT_LIB_DIR}/loc before configuring VT."
  )
endif()

get_target_property(
  VT_LOC_INCLUDE_DIRECTORIES loc::loc INTERFACE_INCLUDE_DIRECTORIES
)

# loc is header-only and does not currently define install rules. Install its
# public headers with VT so installed VT packages do not need the source tree.
install(
  DIRECTORY "${PROJECT_LIB_DIR}/loc/src/loc"
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
