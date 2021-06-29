/*
//@HEADER
// *****************************************************************************
//
//                                  tls.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_UTILS_TLS_TLS_IMPL_H
#define INCLUDED_VT_UTILS_TLS_TLS_IMPL_H

#include "vt/config.h"

namespace vt { namespace util { namespace tls {

#define TagAny(tag,var) tag##_##var##_
#define TagTLS(var) TagAny(tls,var)
#define StrTLS(tag,var) TagAny(tls_static_str,var)
#define DeclareMakeStrTLS(tag, var) char StrTLS(tls,var)[]
#define MakeStrTLS(tag, var, INIT) DeclareMakeStrTLS(tag, var) INIT;
#define InitStrTLS(var) = #var
#define InitTempTLS(init) ,init

#define InnerTLS(TLCLS, TYPE, VAR, INIT, INIT_STR, EXTERN, STATIC)           \
  EXTERN STATIC constexpr MakeStrTLS(tls, VAR, INIT_STR)                     \
  EXTERN STATIC util::tls::TLCLS<TYPE, StrTLS(tls,VAR) INIT> TagTLS(VAR);
#define DeclareInitImplTLS(tlcls, type, var, init)                           \
  InnerTLS(tlcls, type, var, InitTempTLS(init), InitStrTLS(var), , )
#define DeclareImplTLS(tlcls, type, var)                                     \
  InnerTLS(tlcls, type, var, , InitStrTLS(var), , )
#define ExternInitImplTLS(tlcls, type, var, init)                            \
  InnerTLS(tlcls, type, var, InitTempTLS(init), , extern, )
#define ExternImplTLS(tlcls, type, var)                                      \
  InnerTLS(tlcls, type, var, , , extern, )
#define AccessImplTLS(var)                                                   \
  TagTLS(var).get()

// Variants for static file TLS variables
#define DeclareStImplTLS(tlcls, type, var)                                   \
  InnerTLS(tlcls, type, var, , InitStrTLS(var), , static)
#define DeclareStInitImplTLS(tlcls, type, var, init)                         \
  InnerTLS(tlcls, type, var, InitTempTLS(init), InitStrTLS(var), , static)

// Variants for static class TLS variables
#define DeclareMakeStrClsTLS(cls, tag, var, init)                            \
  char cls::StrTLS(tls,var)[] init;
#define MakeStrClsTLS(cls, tag, var, INIT)                                   \
  DeclareMakeStrClsTLS(cls, tag, var) INIT;
#define InnerClsInTLS(TLCLS, CLS, TYPE, VAR, INIT, INIT_STR)                 \
  static DeclareMakeStrTLS(tls, VAR);                                        \
  static util::tls::TLCLS<TYPE, StrTLS(tls,VAR) INIT> TagTLS(VAR);
#define InnerClsOutTLS(TLCLS, CLS, TYPE, VAR, INIT, INIT_STR)                \
  DeclareMakeStrClsTLS(CLS, tls, VAR, INIT_STR)                              \
  util::tls::TLCLS<TYPE, CLS::StrTLS(tls,VAR) INIT> CLS::TagTLS(VAR);

// Variant for static class TLS variables w/o init
#define DeclareClsInImplTLS(tlcls, cls, type, var)                           \
  InnerClsInTLS(tlcls, cls, type, var, , InitStrTLS(var))
#define DeclareClsOutImplTLS(tlcls, cls, type, var)                          \
  InnerClsOutTLS(tlcls, cls, type, var, , InitStrTLS(var))
// Variant for static class TLS variables w init
#define DeclareClsInInitImplTLS(tlcls, cls, type, var, init)                 \
  InnerClsInTLS(tlcls, cls, type, var, InitTempTLS(init), InitStrTLS(var))
#define DeclareClsOutInitImplTLS(tlcls, cls, type, var, init)                \
  InnerClsOutTLS(tlcls, cls, type, var, InitTempTLS(init), InitStrTLS(var))
// Variant for static class TLS variables access
#define AccessClsImplTLS(cls, var)                                           \
  cls::TagTLS(var).get()

}}} /* end namespace vt::util::tls */

#endif /*INCLUDED_VT_UTILS_TLS_TLS_IMPL_H*/
