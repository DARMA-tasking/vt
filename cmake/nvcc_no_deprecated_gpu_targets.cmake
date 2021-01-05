# Add flag "-Wno-deprecated-gpu-targets" for NVCC builds

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "NVIDIA")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-gpu-targets")
endif ()
