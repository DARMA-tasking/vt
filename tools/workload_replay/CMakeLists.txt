
set(
  WORKLOAD_REPLAY_TOOLS
  simulate_replay
)

foreach(TOOL_NAME ${WORKLOAD_REPLAY_TOOLS})
  # message("Example: building workload replay tool >>>>> ${TOOL_NAME}")

  add_tool(${TOOL_NAME})
endforeach()
