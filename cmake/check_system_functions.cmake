
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)

check_include_files(malloc.h vt_has_malloc_h)
check_include_files(malloc/malloc.h vt_has_malloc_malloc_h)
check_include_files(mach/mach.h vt_has_mach_mach_h)
check_include_files(sys/resource.h vt_has_sys_resource_h)
check_include_files(unistd.h vt_has_unistd_h)
check_include_files(inttypes.h vt_has_inttypes_h)
check_include_files(libunwind.h vt_has_libunwind_h)
check_include_files(execinfo.h vt_has_execinfo_h)

check_function_exists(mstats vt_has_mstats)
check_function_exists(popen vt_has_popen)
check_function_exists(pclose vt_has_pclose)
check_function_exists(sbrk vt_has_sbrk)
check_function_exists(getpid vt_has_getpid)
check_function_exists(sysconf vt_has_sysconf)

set(CMAKE_REQUIRED_INCLUDES "malloc.h")
check_function_exists(mallinfo vt_has_mallinfo)
check_function_exists(mallinfo2 vt_has_mallinfo2)

set(CMAKE_REQUIRED_INCLUDES "sys/resource.h")
check_function_exists(getrusage vt_has_getrusage)

set(CMAKE_REQUIRED_INCLUDES "sys/sysinfo.h")
check_function_exists(sysinfo vt_has_sysinfo)

set(CMAKE_REQUIRED_INCLUDES "mach/mach.h")
check_function_exists(mach_task_self vt_has_mach_task_self)

set(CMAKE_REQUIRED_INCLUDES "ldms/ldms.h")
check_function_exists(ldms_h vt_has_ldms_h)

set(CMAKE_REQUIRED_INCLUDES "ldms/ldmsd_stream.h")
check_function_exists(ldmsd_stream_h vt_has_ldmsd_stream_h)

set(CMAKE_REQUIRED_INCLUDES "ovis_util/util.h")
check_function_exists(ovis_util_h vt_has_ovis_util_h)
