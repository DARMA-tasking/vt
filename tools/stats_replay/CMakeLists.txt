
set(
  STATS_REPLAY_TOOLS
  simulate_replay
)

foreach(TOOL_NAME ${STATS_REPLAY_TOOLS})
  # message("Example: building stats replay tool >>>>> ${TOOL_NAME}")

  add_tool(${TOOL_NAME})
endforeach()
