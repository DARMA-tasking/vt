find_package(PAPI REQUIRED)
message(STATUS: "FOUND PAPI: PAPI LIBRARY: ${PAPI_LIBRARY}\n       PAPI INCLUDE DIR: ${PAPI_INCLUDE_DIR}")
set(vt_papi_found "1")
