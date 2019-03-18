#set(HEADER_FILES
#    core.h
#    format-inl.h
#    format.h
#    ostream.h
#)

#install(
#  FILES        ${HEADER_FILES}
#  DESTINATION  include/fmt
#)

install(
  DIRECTORY                "fmt"
  DESTINATION              include
  CONFIGURATIONS           ${build_type_list}
  FILES_MATCHING PATTERN   "*"
)
