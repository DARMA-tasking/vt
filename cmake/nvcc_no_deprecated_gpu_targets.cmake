# Add flag "-Wno-deprecated-gpu-targets" for NVCC builds

if($ENV{CXX} MATCHES "nvcc_wrapper")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-gpu-targets")
endif()
