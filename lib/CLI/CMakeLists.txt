#set(HEADER_FILES
#    CLI11.hpp
#)

#install(
#  FILES        ${HEADER_FILES}
#  DESTINATION  include/CLI
#)

install(
  DIRECTORY                "CLI"
  DESTINATION              include
  CONFIGURATIONS           ${build_type_list}
  FILES_MATCHING PATTERN   "*"
)

