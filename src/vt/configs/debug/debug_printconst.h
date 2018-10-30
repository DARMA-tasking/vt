
#if !defined INCLUDED_DEBUG_PRINT_CONST
#define INCLUDED_DEBUG_PRINT_CONST

#include "meld_headers.h"

/*
 * Specific convienence methods for printing
 */

#define print_bool(BOOL) ((BOOL) ? "true" : "false")

#define print_ptr(PTR) (static_cast<void*>(PTR))

#define print_ptr_const(PTR) (static_cast<void const*>(PTR))

#define print_pool_type(TYPE) (                                \
    (TYPE) == ePoolSize::Small ? "ePoolSize::Small" : (        \
      (TYPE) == ePoolSize::Medium ? "ePoolSize::Medium" : (    \
        (TYPE) == ePoolSize::Large ? "ePoolSize::Large" : (    \
          (TYPE) == ePoolSize::Malloc ? "ePoolSize::Malloc" :  \
          "Unknown"                                            \
        )                                                      \
      )                                                        \
    )                                                          \
  )                                                            \

#endif  /*INCLUDED_DEBUG_PRINT_CONST*/
