set(vt_perf_found "0")

if (vt_perf_enabled)
    # check if linux
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # check if there's the perf header we need
        include(CheckIncludeFiles)
        CHECK_INCLUDE_FILES("linux/perf_event.h" HAVE_PERF_EVENT_H)
        if (HAVE_PERF_EVENT_H)
            # check if the kernel is recent enough
            string(REPLACE "." ";" VERSION_LIST ${CMAKE_SYSTEM_VERSION})
            list(GET VERSION_LIST 0 KERNEL_MAJOR_VERSION)
            if (KERNEL_MAJOR_VERSION GREATER_EQUAL 4)
                # check if a simple perf stat runs without issues
                execute_process(
                    COMMAND perf stat pwd
                    RESULT_VARIABLE PERF_STAT_RESULT
                    OUTPUT_QUIET
                    ERROR_QUIET
                )
                if (PERF_STAT_RESULT EQUAL 0)
                    message(STATUS "Perf measurements enabled.")
                    set(vt_perf_enabled "1")
                    set(vt_perf_found "1")
                else ()
                    message(WARNING "Perf disabled: \"perf stat\" results in non-zero error code.")
                    set(vt_perf_enabled "0")
                    set(vt_perf_found "0")
                endif ()
            else ()
                message(WARNING "Perf disabled: kernel major version is less than 4.")
                set(vt_perf_enabled "0")
                set(vt_perf_found "0")
            endif ()
        else ()
            message(WARNING "Perf disabled: could not find \"perf_event.h\".")
            set(vt_perf_enabled "0")
            set(vt_perf_found "0")
        endif ()
    else ()
        message(WARNING "Perf disabled: system name is not Linux.")
        set(vt_perf_enabled "0")
        set(vt_perf_found "0")
    endif ()
endif ()
