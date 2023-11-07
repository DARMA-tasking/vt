
#
# Tools
#

include(turn_on_warnings)

macro(add_tool tool_name)
  set(TOOL_FILE "${tool_name}.cc")

  add_executable(${tool_name} ${TOOL_FILE})
  add_dependencies(tools ${tool_name})

  turn_on_warnings(${tool_name})

  if (vt_unity_build_enabled)
    set_target_properties(${tool_name} PROPERTIES UNITY_BUILD ON)
  endif()

  link_target_with_vt(
    TARGET ${tool_name}
    DEFAULT_LINK_SET
  )
endmacro()

add_subdirectory(workload_replay)
