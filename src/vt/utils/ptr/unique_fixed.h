/*
//@HEADER
// *****************************************************************************
//
//                                unique_fixed.h
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

#if !defined INCLUDED_VT_UTILS_PTR_UNIQUE_FIXED_H
#define INCLUDED_VT_UTILS_PTR_UNIQUE_FIXED_H

#include "vt/pool/static_sized/memory_pool_equal.h"

namespace vt { namespace util { namespace ptr {

template <typename T>
using unique_ptr_fixed = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T, int64_t num_bytes, typename... Args>
unique_ptr_fixed<T> make_unique_fixed(pool::MemoryPoolEqual<num_bytes>& pool, Args&&... args) {
  vtAssert(sizeof(T) <= num_bytes, "Must fit in pool");
  T* ptr = new (static_cast<T*>(pool.alloc(sizeof(T), 0))) T{std::forward<Args>(args)...};
  auto deleter = [&pool](T* p) {
    p->~T();
    pool.dealloc(static_cast<void*>(p));
  };
  return std::unique_ptr<T, std::function<void(T*)>>(ptr, deleter);
}

template <typename B, int64_t num_bytes, typename T>
unique_ptr_fixed<B> unique_fixed_to_base(
  pool::MemoryPoolEqual<num_bytes>& pool, unique_ptr_fixed<T>&& up
) {
  auto t = up.release();
  auto deleter = [&pool](B* p) {
    auto x = static_cast<T*>(p);
    x->~T();
    pool.dealloc(static_cast<void*>(p));
  };
  return std::unique_ptr<B, std::function<void(B*)>>(t, deleter);
}

}}} /* end namespace vt::util::ptr */

#endif /*INCLUDED_VT_UTILS_PTR_UNIQUE_FIXED_H*/
