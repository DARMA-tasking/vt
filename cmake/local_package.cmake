
macro(require_pkg_directory pkg_name pkg_user_name)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    # Skip this logic when this macro was not invoked from the
    # top-level CMakeLists.txt file under the assumption that this
    # package was dropped into another build system using add_subdirectory().
    # Note that this will also skip if you call this macro from
    # a subdirectory in your own package, so just don't do it!

    #message(STATUS "skipping require_pkg_directory for ${pkg_name}")
  else()
    #message(STATUS "require_directory: name=${pkg_name}")
    option(${pkg_name}_DIR "Root folder for ${pkg_user_name} installation" OFF)
    if (NOT ${pkg_name}_DIR)
      message(
        FATAL_ERROR
        "Please specify ${pkg_user_name} library installation root"
        " with -D${pkg_name}_DIR="
      )
    endif()
  endif()
endmacro(require_pkg_directory)

macro(find_package_local pkg_name pkg_directory pkg_other_name)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    # Skip this logic when this macro was not invoked from the
    # top-level CMakeLists.txt file under the assumption that this
    # package was dropped into another build system using add_subdirectory().
    # Note that this will also skip if you call this macro from
    # a subdirectory in your own package, so just don't do it!

    #message(STATUS "skipping find_package for ${pkg_name}")
  else()
    message(
      STATUS "find_package_local: pkg name=\"${pkg_name}\", "
             "directory=\"${pkg_directory}\""
    )

    # Rest of the arguments are potential relative search paths wrt the
    # ${pkg_directory}
    set(prefix_args ${ARGN})

    # Default search paths: root, /cmake and /CMake subdirectories
    list(APPEND prefix_args "/" "/cmake" "/CMake")

    # Whether we loaded the package in the following loop with find_package()
    set(${pkg_name}_PACKAGE_LOADED 0)

    foreach(prefix ${prefix_args})
      set(potential_path ${pkg_directory}/${prefix})
      # message("prefix: ${potential_path}")
      if (EXISTS "${potential_path}")
        # message(STATUS "find_package_local: trying path: ${potential_path}")

        # Search locally only for package based on the user's supplied path; if
        # this fails try to next one. Even if the directory exists (tested above)
        # this might fail if a directory does not have the config file
        find_package(
          ${pkg_name}
          PATHS ${potential_path}
          NAMES ${pkg_name} ${pkg_other_name}
          NO_CMAKE_PACKAGE_REGISTRY
          NO_CMAKE_BUILDS_PATH
          NO_CMAKE_SYSTEM_PATH
          NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
          NO_SYSTEM_ENVIRONMENT_PATH
          QUIET
        )

        # Break out of the search loop now that we have found the path
        if (${${pkg_name}_FOUND})
          message(STATUS "find_package_local: found with prefix: ${prefix}")
          set(${pkg_name}_PACKAGE_LOADED 1)
          break()
        endif()
      endif()
    endforeach()

    if (NOT ${${pkg_name}_PACKAGE_LOADED})
      message(STATUS "find_package_local: can not find package: ${pkg_name}")

      foreach(prefix ${prefix_args})
        set(path ${${pkg_name}_DIR}/${prefix})
        message(STATUS "find_package_local: searched: ${path}")
      endforeach()

      message(
        FATAL_ERROR "find_package_local: can not find package: ${pkg_name}"
        " tried to find_package(..) with above search paths"
      )
    endif()
  endif()
endmacro(find_package_local)

macro(optional_pkg_directory pkg_name pkg_user_name assume_found_if_hasparent)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    # Skip MOST of this logic when this macro was not invoked from the
    # top-level CMakeLists.txt file under the assumption that this
    # package was dropped into another build system using add_subdirectory().
    # Note that this will also skip if you call this macro from
    # a subdirectory in your own package, so just don't do it!

    if(${assume_found_if_hasparent})
      # Assume that the package is available even if the directory wasn't specified
      set(${pkg_name}_DIR_FOUND 1)
    endif()
  else()
    #message(STATUS "optional_pkg_directory: name=${pkg_name}")
    option(${pkg_name}_DIR "Root folder for ${pkg_user_name} installation" OFF)
    if (NOT ${pkg_name}_DIR)
      message(
        STATUS
        "Path for ${pkg_user_name} library (optional) not specified "
        "with -D${pkg_name}_DIR="
      )
      message(
        STATUS
        "Building without ${pkg_user_name} library"
      )
      set(${pkg_name}_DIR_FOUND 0)
    else()
      message(
        STATUS
        "Path for ${pkg_user_name} library (optional) specified "
        "with -D${pkg_name}_DIR=${${pkg_name}_DIR}"
      )
      set(${pkg_name}_DIR_FOUND 1)
    endif()
  endif()
endmacro(optional_pkg_directory)
