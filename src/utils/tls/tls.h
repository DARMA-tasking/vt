
#if !defined INCLUDED_UTILS_TLS_TLS_H
#define INCLUDED_UTILS_TLS_TLS_H

#include "config.h"

#define backend_null_tls 1

#if backend_check_enabled(openmp)
  #include "utils/tls/omp_tls.h"
#elif backend_check_enabled(stdthread)
  #include "utils/tls/std_tls.h"
#elif backend_no_threading
  #include "utils/tls/null_tls.h"
#else
  backend_static_assert_unreachable
#endif

#if backend_null_tls
  #include "utils/tls/null_tls.h"
#endif

namespace vt { namespace util { namespace tls {

#if backend_check_enabled(openmp)
  template <typename T, char const* tag>
  using ThreadLocalType = ThreadLocalOMP<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalInitType = ThreadLocalInitOMP<T,tag,val>;
#elif backend_check_enabled(stdthread)
  template <typename T, char const* tag>
  using ThreadLocalType = ThreadLocalSTD<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalInitType = ThreadLocalSTDInit<T,tag,val>;
#elif backend_no_threading
  template <typename T, char const* tag>
  using ThreadLocalType = ThreadLocalNull<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalInitType = ThreadLocalInitNull<T,tag,val>;
#else
  backend_static_assert_unreachable
#endif

#if backend_null_tls
  template <typename T, char const* tag>
  using ThreadLocalNullType = ThreadLocalNull<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalNullInitType = ThreadLocalInitNull<T,tag,val>;
#endif

// Declare and extern TLS variable with initializer
#define DeclareInitTLS(type, var, init)                     \
  DeclareInitImplTLS(ThreadLocalInitType, type, var, init)
#define ExternInitTLS(type, var, init)                    \
  ExternInitImplTLS(ThreadLocalInitType, type, var, init)
// Declare and extern TLS variable w/o initializer
#define DeclareTLS(type, var)                   \
  DeclareImplTLS(ThreadLocalType, type, var)
#define ExternTLS(type, var)                    \
  ExternImplTLS(ThreadLocalType, type, var)
// Access a TLS variable
#define AccessTLS(var)                          \
  AccessImplTLS(var)

// Declare and extern a variable with initializer that may or may not be TLS
#define DeclareInitVar(TLS, type, var, init)                            \
  DeclareInitImplTLS(                                                   \
    ThreadLocalInitType, type, var, init                                \
  )
#define ExternInitVar(TLS, type, var, init)                             \
  ExternInitImplTLS(                                                    \
    ThreadLocalInitType, type, var, init                                \
  )
#define AccessVar(var)                          \
  AccessImplTLS(var)

// Variant for file-level static TLS variables
#define DeclareStaticTLS(type, var)             \
  DeclareStImplTLS(ThreadLocalType, type, var)
#define DeclareStaticInitTLS(type, var, init)                 \
  DeclareStInitImplTLS(ThreadLocalInitType, type, var, init)

// Variant for class level static TLS variables
// Declare class level static TLS variable w/o initializer
#define DeclareClassInsideTLS(cls, type, var)           \
  DeclareClsInImplTLS(ThreadLocalType, cls, type, var)
#define DeclareClassOutsideTLS(cls, type, var)              \
  DeclareClsOutImplTLS(ThreadLocalType, cls, type, var)

// Declare class level static TLS variable w initializer
#define DeclareClassInsideInitTLS(cls, type, var, init)               \
  DeclareClsInInitImplTLS(ThreadLocalInitType, cls, type, var, init)
#define   DeclareClassOutsideInitTLS(cls, type, var, init)            \
  DeclareClsOutInitImplTLS(ThreadLocalInitType, cls, type, var, init)

// Access a static TLS variable in a class
#define AccessClassTLS(cls, var)                \
  AccessClsImplTLS(cls, var)

}}} /* end namespace vt::util::tls */

#include "tls.impl.h"

#endif /*INCLUDED_UTILS_TLS_TLS_H*/
