
#if !defined INCLUDED_UTILS_TLS_TLS_H
#define INCLUDED_UTILS_TLS_TLS_H

#include "config.h"

#if backend_check_enabled(openmp)
  #include <omp.h>
  #include "utils/tls/omp_tls.h"
#else
  #include "utils/tls/std_tls.h"
#endif

namespace vt { namespace util { namespace tls {

#if backend_check_enabled(openmp)
  template <typename T, char const* tag, T val = T{}>
  using ThreadLocalType = ThreadLocalOMP<T,tag,val>;
#else
  template <typename T, char const* tag, T val = T{}>
  using ThreadLocalType = ThreadLocalSTD<T,tag,val>;
#endif

// Declare and extern TLS variable with initializer
#define DeclareInitTLS(type, var, init) DeclareInitImplTLS(type, var, init)
#define ExternInitTLS(type, var, init)  ExternInitImplTLS(type, var, init)
// Declare and extern TLS variable w/o initializer
#define DeclareTLS(type, var)           DeclareImplTLS(type, var)
#define ExternTLS(type, var)            ExternImplTLS(type, var)
// Access a TLS variable
#define AccessTLS(var)                  AccessImplTLS(var)

}}} /* end namespace vt::util::tls */

#include "tls.impl.h"

#endif /*INCLUDED_UTILS_TLS_TLS_H*/
