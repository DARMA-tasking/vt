/*
//@HEADER
// *****************************************************************************
//
//                                   index.h
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

#if !defined INCLUDED_TOPOS_INDEX
#define INCLUDED_TOPOS_INDEX

#include <cstdint>

#include "vt/config.h"
#include "vt/topos/index/index_example.h"
#include "vt/topos/index/dense/dense_array.h"

#if backend_check_enabled(detector)
  #include "vt/topos/index/traits/traits.h"
#endif

namespace vt { namespace index {

using IdxBase = int32_t;

template <typename T = IdxBase> using Index1D  = DenseIndexArray<T, 1>;
template <typename T = IdxBase> using Index2D  = DenseIndexArray<T, 2>;
template <typename T = IdxBase> using Index3D  = DenseIndexArray<T, 3>;
template <typename T, int8_t N> using IdxType = DenseIndexArray<T, N>;

#if backend_check_enabled(detector)
  static_assert(IndexTraits<Index1D<IdxBase>>::is_index, "Does not conform");
  static_assert(IndexTraits<Index2D<IdxBase>>::is_index, "Does not conform");
  static_assert(IndexTraits<Index3D<IdxBase>>::is_index, "Does not conform");
#endif

}}  // end namespace vt::index

namespace vt {

template <typename IndexType, index::NumDimensionsType ndim>
using DenseIndex = index::DenseIndexArray<IndexType, ndim>;
using IdxBase    = index::IdxBase;

using Index1D  = index::Index1D<index::IdxBase>;
using Index2D  = index::Index2D<index::IdxBase>;
using Index3D  = index::Index3D<index::IdxBase>;

template <typename T, int8_t N> using IdxType   = index::IdxType<T, N>;
template <typename T>           using IdxType1D = index::Index1D<T>;
template <typename T>           using IdxType2D = index::Index2D<T>;
template <typename T>           using IdxType3D = index::Index3D<T>;

}  // end namespace vt

#include "vt/topos/index/printer/print_index.h"

#endif  /*INCLUDED_TOPOS_INDEX*/
