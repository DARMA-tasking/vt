
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

#define InnerTLS(TYPE, VAR, INIT, INIT_STR, EXTERN, STATIC)             \
  EXTERN STATIC MakeStrTLS(tls, VAR, INIT_STR)                          \
  EXTERN STATIC util::tls::ThreadLocalType<TYPE, StrTLS(tls,VAR) INIT> TagTLS(VAR);
#define DeclareInitImplTLS(type, var, init)   InnerTLS(type, var, InitTempTLS(init), InitStrTLS(var), , )
#define DeclareImplTLS(type, var)             InnerTLS(type, var, , InitStrTLS(var), , )
#define ExternInitImplTLS(type, var, init)    InnerTLS(type, var, InitTempTLS(init), , extern, )
#define ExternImplTLS(type, var)              InnerTLS(type, var, , , extern, )
#define AccessImplTLS(var)                    decltype(TagTLS(var))::get()

// Variants for static file TLS variables
#define DeclareStImplTLS(type, var)           InnerTLS(type, var, , InitStrTLS(var), , static)
#define DeclareStInitImplTLS(type, var, init) InnerTLS(type, var, InitTempTLS(init), InitStrTLS(var), , static)

// Variants for static class TLS variables
#define DeclareMakeStrClsTLS(cls, tag, var, init) char cls::StrTLS(tls,var)[] init;
#define MakeStrClsTLS(cls, tag, var, INIT) DeclareMakeStrClsTLS(cls, tag, var) INIT;
#define InnerClsInTLS(CLS, TYPE, VAR, INIT, INIT_STR)                   \
  static DeclareMakeStrTLS(tls, VAR);                                   \
  static util::tls::ThreadLocalType<TYPE, StrTLS(tls,VAR) INIT> TagTLS(VAR);
#define InnerClsOutTLS(CLS, TYPE, VAR, INIT, INIT_STR)                  \
  DeclareMakeStrClsTLS(CLS, tls, VAR, INIT_STR)                         \
  util::tls::ThreadLocalType<TYPE, CLS::StrTLS(tls,VAR) INIT> CLS::TagTLS(VAR);

// Variant for static class TLS variables w/o init
#define DeclareClsInImplTLS(cls, type, var)            InnerClsInTLS(cls, type, var, , InitStrTLS(var))
#define DeclareClsOutImplTLS(cls, type, var)           InnerClsOutTLS(cls, type, var, , InitStrTLS(var))
// Variant for static class TLS variables w init
#define DeclareClsInInitImplTLS(cls, type, var, init)  InnerClsInTLS(cls, type, var, InitTempTLS(init), InitStrTLS(var))
#define DeclareClsOutInitImplTLS(cls, type, var, init) InnerClsOutTLS(cls, type, var, InitTempTLS(init), InitStrTLS(var))
// Variant for static class TLS variables access
#define AccessClsImplTLS(cls, var)                     decltype(cls::TagTLS(var))::get()

}}} /* end namespace vt::util::tls */

#endif /*INCLUDED_UTILS_TLS_TLS_IMPL_H*/
