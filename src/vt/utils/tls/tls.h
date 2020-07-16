/*
//@HEADER
// *****************************************************************************
//
//                                    tls.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_UTILS_TLS_TLS_H
#define INCLUDED_UTILS_TLS_TLS_H

#include "vt/config.h"

#define backend_null_tls 1

#if vt_check_enabled(openmp)
  #include "vt/utils/tls/omp_tls.h"
#elif vt_check_enabled(stdthread)
  #include "vt/utils/tls/std_tls.h"
#elif backend_no_threading
  #include "vt/utils/tls/null_tls.h"
#else
  backend_static_assert_unreachable
#endif

#if backend_null_tls
  #include "vt/utils/tls/null_tls.h"
#endif

namespace vt { namespace util { namespace tls {

#if vt_check_enabled(openmp)
  template <typename T, char const* tag>
  using ThreadLocalType = ThreadLocalOMP<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalInitType = ThreadLocalInitOMP<T,tag,val>;
#elif vt_check_enabled(stdthread)
  template <typename T, char const* tag>
  using ThreadLocalType = ThreadLocalSTD<T,tag>;

  template <typename T, char const* tag, T val>
  using ThreadLocalInitType = ThreadLocalInitSTD<T,tag,val>;
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
#define DeclareInitTLS(type, var, init)                                       \
  DeclareInitImplTLS(ThreadLocalInitType, type, var, init)
#define ExternInitTLS(type, var, init)                                        \
  ExternInitImplTLS(ThreadLocalInitType, type, var, init)
// Declare and extern TLS variable w/o initializer
#define DeclareTLS(type, var)                                                 \
  DeclareImplTLS(ThreadLocalType, type, var)
#define ExternTLS(type, var)                                                  \
  ExternImplTLS(ThreadLocalType, type, var)
// Access a TLS variable
#define AccessTLS(var)                                                        \
  AccessImplTLS(var)

// Declare and extern a variable with initializer that may or may not be TLS
#define DeclareInitVar(TLS, type, var, init)                                  \
  DeclareInitImplTLS(                                                         \
    ThreadLocalInitType, type, var, init                                      \
  )
#define ExternInitVar(TLS, type, var, init)                                   \
  ExternInitImplTLS(                                                          \
    ThreadLocalInitType, type, var, init                                      \
  )
#define AccessVar(var)                                                        \
  AccessImplTLS(var)

// Variant for file-level static TLS variables
#define DeclareStaticTLS(type, var)                                           \
  DeclareStImplTLS(ThreadLocalType, type, var)
#define DeclareStaticInitTLS(type, var, init)                                 \
  DeclareStInitImplTLS(ThreadLocalInitType, type, var, init)

// Variant for class level static TLS variables
// Declare class level static TLS variable w/o initializer
#define DeclareClassInsideTLS(cls, type, var)                                 \
  DeclareClsInImplTLS(ThreadLocalType, cls, type, var)
#define DeclareClassOutsideTLS(cls, type, var)                                \
  DeclareClsOutImplTLS(ThreadLocalType, cls, type, var)

// Declare class level static TLS variable w initializer
#define DeclareClassInsideInitTLS(cls, type, var, init)                       \
  DeclareClsInInitImplTLS(ThreadLocalInitType, cls, type, var, init)
#define DeclareClassOutsideInitTLS(cls, type, var, init)                      \
  DeclareClsOutInitImplTLS(ThreadLocalInitType, cls, type, var, init)

// Access a static TLS variable in a class
#define AccessClassTLS(cls, var)                                              \
  AccessClsImplTLS(cls, var)

}}} /* end namespace vt::util::tls */

#include "vt/utils/tls/tls.impl.h"

#endif /*INCLUDED_UTILS_TLS_TLS_H*/
