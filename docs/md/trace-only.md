\page trace-only Trace-only
\brief Using vt in trace-only mode

\section what-is-vt-trace What is trace-only mode

Trace-only mode is lightweight library which includes the subset of VT-runtime that allows to generate trace files for an application that uses MPI calls.
The following examples will show how to generate vt-trace target and then how to use it.

\section build Build vt with trace-only target:

Depending on how you build VT, you can either set environment variable `VT_BUILD_TRACE_ONLY`:
```cmake
export VT_BUILD_TRACE_ONLY=1
/vt/ci/build_cpp.sh /vt /build
```

or add CMake flag `vt_trace_only`:


```cmake
cmake -Dvt_trace_only=1
```

This will generate vt-trace target alongside the regular vt-runtime CMake target.

***

\section usage Example usage:

## CMake
Next step is to setup CMake. Find the `vt` project and load its settings:


\code{.cmake}
find_package(vt REQUIRED)
\endcode

Then link `vt-trace` library with your target:


\code{.cmake}
target_link_libraries(${target_name} PUBLIC vt::vt-trace)
\endcode


Example CMake:
\code{.cmake}
cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(my_target)

find_package(vt REQUIRED)

add_executable(
  my_target
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sample.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sample.cc
)

target_link_libraries(my_target PUBLIC vt::vt-trace)
\endcode

## Source code

The following code snippet shows the example use of vt-trace library (See `vt::trace::TraceLite` for more information):

\code{.cpp}
#include <mpi.h>
#include "vt/trace/trace_lite.h"

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  // Create vt::trace::TraceLite object after MPI has been initialized
  // The constructor takes one argument - your application's name
  ::vt::trace::TraceLite myTrace("MyApp");


  // Call initializeStandalone once to initialize all internal data needed for tracing
  // This function takes single argument - MPI communicator
  myTrace.initializeStandalone(MPI_COMM_WORLD);

  /*
     DO MPI RELATED WORK HERE
  */

  // Use this function to flush traces to file
  myTrace.flushTracesFile();

  // After all work is done, call this function to cleanup all data
  // This should be called before calling 'MPI_Finalize'
  myTrace.finalizeStandalone();

  MPI_Finalize();
}
\endcode
