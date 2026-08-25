include(FetchContent)

set(
  VT_LOC_GIT_REPOSITORY
  "https://github.com/DARMA-tasking/loc.git"
  CACHE STRING
  "Git repository used to obtain DARMA/loc"
)
set(
  VT_LOC_GIT_TAG
  "0649b54065cfd566549e38a3a5c96f417fd513ca"
  CACHE STRING
  "DARMA/loc commit, tag, or branch used by VT"
)

if (NOT TARGET loc::loc)
  FetchContent_Declare(
    loc
    GIT_REPOSITORY "${VT_LOC_GIT_REPOSITORY}"
    GIT_TAG "${VT_LOC_GIT_TAG}"
    GIT_PROGRESS TRUE
  )

  # loc detects that it is embedded and disables its standalone comm/MPI
  # backend, examples, tests, and documentation. VT supplies LocCommunicator.
  FetchContent_MakeAvailable(loc)
endif()

get_target_property(
  VT_LOC_INCLUDE_DIRECTORIES loc::loc INTERFACE_INCLUDE_DIRECTORIES
)
