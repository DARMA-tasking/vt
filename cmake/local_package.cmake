macro(find_package_local pkg_name pkg_other_name)
  if(hasParent)
    # Skip this logic when this macro was not invoked from the
    # top-level CMakeLists.txt file under the assumption that this
    # package was dropped into another build system using add_subdirectory().
    # Note that this will also skip if you call this macro from
    # a subdirectory in your own package, so just don't do it!

    # message(STATUS "skipping find_package for ${pkg_name}")
  else()
    message(STATUS "find_package_local: pkg name=\"${pkg_name}\"")

    find_package(
      ${pkg_name}
      NAMES ${pkg_name} ${pkg_other_name}
      NO_CMAKE_PACKAGE_REGISTRY
      NO_CMAKE_BUILDS_PATH
      NO_CMAKE_SYSTEM_PATH
      NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
      NO_SYSTEM_ENVIRONMENT_PATH
      QUIET
      REQUIRED
    )
  endif()
endmacro(find_package_local)
