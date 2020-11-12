
include(GetGitRevisionDescription)

# set some variables related to GIT state information
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_EXACT_TAG --tags --abbrev=0 --all)
git_describe(GIT_DESCRIPTION --abbrev=10 --always --tags --long --all)
git_local_changes(GIT_CLEAN_STATUS)

message(STATUS "REF:${GIT_REFSPEC}")
message(STATUS "REF:${GIT_SHA1}")
message(STATUS "REF:${GIT_DESCRIPTION}")
message(STATUS "REF:${GIT_CLEAN_STATUS}")
message(STATUS "REF:${GIT_EXACT_TAG}")

configure_file(
  ${PROJECT_BASE_DIR}/vt_git_revision.cc.in
  ${PROJECT_BIN_DIR}/src/vt/configs/generated/vt_git_revision.cc
  @ONLY
)

# install(
#   FILES            "${PROJECT_BINARY_DIR}/${cur_build_type}/cmake_config.h"
#   DESTINATION      include
#   CONFIGURATIONS   ${cur_build_type}
# )

# configure_file(
#   "${PROJECT_SOURCE_DIR}/vt_git_revision.cc.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/vt_git_revision.cc"
#   @ONLY
# )
