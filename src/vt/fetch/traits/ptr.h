/*
//@HEADER
// ************************************************************************
//
//                          ptr.h
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

#if !defined INCLUDED_VT_FETCH_TRAITS_PTR_H
#define INCLUDED_VT_FETCH_TRAITS_PTR_H

#include "vt/config.h"

#include <vector>
#include <type_traits>

namespace vt { namespace fetch {

template <typename T>
struct PtrTraits;

template <typename T>
struct PtrTraits<T*> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T, std::size_t N>
struct PtrTraits<T[N]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<T[]> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1 + PtrTraits<T>::dims;
};

template <typename T>
struct PtrTraits<std::vector<T>> {
  using BaseType = typename PtrTraits<T>::BaseType;
  static constexpr bool arith = PtrTraits<T>::arith;
  static constexpr bool vec = true;
  static constexpr bool array = false;
  static constexpr std::size_t dims = 1;
};

template <typename T>
struct PtrTraits {
  using BaseType = T;
  static constexpr bool arith = std::is_arithmetic<T>::value;
  static constexpr bool vec = false;
  static constexpr bool array = false;
  static constexpr std::size_t dims = PtrTraits<T>::dims;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_TRAITS_PTR_H*/
