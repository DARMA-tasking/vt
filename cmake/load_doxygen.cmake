
option(vt_doxygen_enabled "Build doxygen documentation for VT" OFF)


if (${vt_doxygen_enabled})
  find_package(Doxygen)

  if (DOXYGEN_FOUND)
    set(doxygen_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(doxygen_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    set(DOXYGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/")
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)

    configure_file(${doxygen_in} ${doxygen_out} @ONLY)
    configure_file(${doxygen_in}-mcss ${doxygen_out}-mcss @ONLY)
    message(STATUS "VT doxygen build started")

    add_custom_target(
      docs ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${doxygen_out}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "VT generating API documentation with Doxygen"
      VERBATIM
    )

  #   find_package(Sphinx REQUIRED)

  #   set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
  #   set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
  #   set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

  #   add_custom_command(
  #     OUTPUT ${SPHINX_INDEX_FILE}
	#     COMMAND ${SPHINX_EXECUTABLE} -b html
	# 	  # Tell Breathe where to find the Doxygen output
	# 	  -Dbreathe_projects.vt=${CMAKE_CURRENT_BINARY_DIR}/docs/xml
	# 	  ${SPHINX_SOURCE} ${SPHINX_BUILD}
	#     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	#     DEPENDS
	# 	  # Other docs files you want to track should go here (or in some variable)
	# 	  ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
	# 	  ${DOXYGEN_INDEX_FILE}
	#     MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
	#     COMMENT "Generating documentation with Sphinx"
  #   )

  # # Nice named target so we can run the job easily
  # add_custom_target(sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

  else()
    message(FATAL_ERROR "Doxygen could not be found even though it was enabled")
  endif()
endif()
