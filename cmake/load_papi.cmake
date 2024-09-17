set(vt_papi_found "0")

if (vt_papi_enabled)
    if (vt_perf_enabled)
        set(vt_papi_found "0")
        set(vt_papi_enabled "0")
        message(FATAL_ERROR "Both PAPI and perf measurements are enabled; this will cause errors, please turn off one of these options. Exiting.")
    endif ()
    find_package(PAPI REQUIRED)
    message(STATUS: "FOUND PAPI: PAPI LIBRARY: ${PAPI_LIBRARY}\n       PAPI INCLUDE DIR: ${PAPI_INCLUDE_DIR}")
    set(vt_papi_found "1")
endif ()