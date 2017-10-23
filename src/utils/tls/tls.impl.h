
#if !defined INCLUDED_UTILS_TLS_TLS_IMPL_H
#define INCLUDED_UTILS_TLS_TLS_IMPL_H

#include "config.h"

namespace vt { namespace util { namespace tls {

#define TagAny(tag,var) tag##_##var##_
#define TagTLS(var) TagAny(tls,var)
#define StrTLS(tag,var) TagAny(tls_static_str,var)
#define DeclareMakeStrTLS(tag, var) char StrTLS(tls,var)[]
#define MakeStrTLS(tag, var, INIT) DeclareMakeStrTLS(tag, var) INIT;
#define InitStrTLS(var) = #var
#define InitTempTLS(init) ,init

#define InnerTLS(TYPE, VAR, INIT, INIT_STR, EXTERN)                     \
  EXTERN MakeStrTLS(tls, VAR, INIT_STR)                                 \
  EXTERN util::tls::ThreadLocalType<TYPE, StrTLS(tls,VAR) INIT> TagTLS(VAR);
#define DeclareInitImplTLS(type, var, init) InnerTLS(type, var, InitTempTLS(init), InitStrTLS(var), )
#define DeclareImplTLS(type, var)           InnerTLS(type, var, , InitStrTLS(var), )
#define ExternInitImplTLS(type, var, init)  InnerTLS(type, var, InitTempTLS(init), , extern)
#define ExternImplTLS(type, var)            InnerTLS(type, var, , , extern)
#define AccessImplTLS(var)                  decltype(TagTLS(var))::get()

}}} /* end namespace vt::util::tls */

#endif /*INCLUDED_UTILS_TLS_TLS_IMPL_H*/
