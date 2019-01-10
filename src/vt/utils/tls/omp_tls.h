/*
//@HEADER
// ************************************************************************
//
//                          omp_tls.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_UTILS_TLS_OMP_TLS_H
#define INCLUDED_UTILS_TLS_OMP_TLS_H

#include "vt/config.h"

#if backend_check_enabled(openmp)
#include <omp.h>

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val>
struct ThreadLocalInitOMP {
  static T& get() { return value_; }
private:
  #if defined(__GNUC__)
    static thread_local T value_;
  #else
    static T value_;
    #pragma omp threadprivate (value_)
  #endif
};

template <typename T, char const* tag>
struct ThreadLocalOMP {
  static T& get() { return value_; }
private:
  #if defined(__GNUC__)
    static thread_local T value_;
  #else
    static T value_;
    #pragma omp threadprivate (value_)
  #endif
};

#if defined(__GNUC__)
  template <typename T, char const* tag, T val>
  T thread_local ThreadLocalInitOMP<T,tag,val>::value_ = val;

  template <typename T, char const* tag>
  T thread_local ThreadLocalOMP<T,tag>::value_;
#else
  template <typename T, char const* tag, T val>
  T ThreadLocalInitOMP<T,tag,val>::value_ = val;

  template <typename T, char const* tag>
  T ThreadLocalOMP<T,tag>::value_;
#endif

}}} /* end namespace vt::util::tls */

#endif /*backend_check_enabled(openmp)*/
#endif /*INCLUDED_UTILS_TLS_OMP_TLS_H*/
