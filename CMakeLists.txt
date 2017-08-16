cmake_minimum_required(VERSION 3.8)
project(virtual_transport_mpi)

find_package(MPI REQUIRED)
if (MPI_FOUND)
    #        set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
    #        set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
    include_directories(MPI_INCLUDE_PATH)
else (MPI_FOUND)
    message(SEND_ERROR "This application cannot compile without MPI")
endif (MPI_FOUND)

set(CMAKE_CXX_STANDARD 14)


set(SOURCE_FILES
        active.cc
        collective.cc
        event.cc
        runtime.cc
        termination.cc
        transport.cc
        )

set(HEADER_FILES
        active.h
        collective.h
        common.h
        context.h
        debug.h
        envelope.h
        event.h
        event_msgs.h
        message.h
        registry.h
        termination.h
        transport.h
        virual_context.h
        )


add_executable(vt_mpi ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(vt_mpi
        PRIVATE
        ${MPI_C_LIBRARIES})

target_include_directories(vt_mpi
        PRIVATE
        ${MPI_C_INCLUDE_PATH})