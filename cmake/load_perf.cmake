set(vt_perf_found "0")

if (vt_perf_enabled)
    if (vt_papi_enabled)
        message(FATAL_ERROR "Both PAPI and perf measurements are enabled; this will cause errors, please turn off one of these options. Exiting.")
    endif ()
    # check if linux
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # check if there's the perf header we need
        INCLUDE(CheckIncludeFiles)
        CHECK_INCLUDE_FILES("linux/perf_event.h" HAVE_PERF_EVENT_H)
        if (HAVE_PERF_EVENT_H)
            # check if the kernel is recent enough
            string(REPLACE "." ";" VERSION_LIST ${CMAKE_SYSTEM_VERSION})
            list(GET VERSION_LIST 0 KERNEL_MAJOR_VERSION)
            if (KERNEL_MAJOR_VERSION GREATER_EQUAL 4)
                # # check if a simple perf stat runs without issues
                # execute_process(
                #     COMMAND "perf stat pwd"
                #     RESULT_VARIABLE PERF_STAT_RESULT
                #     OUTPUT_QUIET
                #     ERROR_QUIET
                # )
                # if (PERF_STAT_RESULT EQUAL 0)
                    message(STATUS "Perf measurements enabled.")
                    set(vt_perf_enabled "1")
                    set(vt_perf_found "1")
                # else ()
                #     message(WARNING "Perf measurements enabled but couldn't run perf stat successfully. Disabling perf measurements.")
                #     set(vt_perf_enabled "0")
                #     set(vt_perf_found "0")
                # endif ()
            else ()
                message(WARNING "Perf measurements enabled but Kernel major version is less than 4. Disabling perf measurements.")
                set(vt_perf_enabled "0")
                set(vt_perf_found "0")
            endif ()
        else ()
            message(WARNING "Perf measurements enabled but couldn't find perf_event.h. Disabling perf measurements.")
            set(vt_perf_enabled "0")
            set(vt_perf_found "0")
        endif ()
    else ()
        message(WARNING "Perf measurements enabled but system is not Linux. Disabling perf measurements.")
        set(vt_perf_enabled "0")
        set(vt_perf_found "0")
    endif ()
endif ()
