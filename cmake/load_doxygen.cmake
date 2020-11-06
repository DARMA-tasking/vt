
option(vt_doxygen_enabled "Build doxygen documentation for VT" OFF)


if (${vt_doxygen_enabled})
  find_package(Doxygen)

  if (DOXYGEN_FOUND)
    set(doxygen_in ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(doxygen_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    set(DOXYGEN_PROJECT_NAME "vt")
    set(VERSION_MAJOR "1")
    set(VERSION_MINOR "0")
    set(VERSION_PATCH "0")
    set(DOXYGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
    set(DOXYGEN_CHECKPOINT_DOCS_DIR " ${CMAKE_CURRENT_SOURCE_DIR}/lib/checkpoint/docs")
    set(DOXYGEN_CHECKPOINT_EXAMPLE_DIR " ${CMAKE_CURRENT_SOURCE_DIR}/lib/checkpoint/examples")
    set(DOXYGEN_CHECKPOINT_SRC_DIR " ${CMAKE_CURRENT_SOURCE_DIR}/lib/checkpoint/src")
    set(DOXYGEN_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/")
    set(DOXYGEN_EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples/")
    set(DOXYGEN_TUTORIAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tutorial/")
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/")
    set(DOXYGEN_MAIN_PAGE "${CMAKE_CURRENT_SOURCE_DIR}/src/vt.md")
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

  else()
    message(FATAL_ERROR "Doxygen could not be found even though it was enabled")
  endif()
endif()
