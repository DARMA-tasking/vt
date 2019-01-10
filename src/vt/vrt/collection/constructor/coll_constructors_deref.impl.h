/*
//@HEADER
// ************************************************************************
//
//                          coll_constructors_deref.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.h"

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename Tuple, typename... Args>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
DerefCons::derefTuple(
  VirtualElmCountType const& elms, IndexT const& idx, std::tuple<Args...>* tup
) {
  static constexpr auto size = std::tuple_size<Tuple>::value;
  static constexpr auto seq = std::make_index_sequence<size>{};
  using DispatcherType = DispatchCons<
    ColT, IndexT, typename CollectionManager::VirtualPtrType<ColT, IndexT>,
    Tuple, Args...
  >;
  return expandSeq<ColT, IndexT, Tuple, DispatcherType>(elms, idx, tup, seq);
}

template <
  typename ColT, typename IndexT, typename Tuple, typename DispatcherT,
  size_t... I
>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
DerefCons::expandSeq(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...> seq
) {
  using ConsType = typename DispatcherT::template ConsType<I...>;
  ConsType cons;
  return cons(elms, idx, tup, seq);
}

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_IMPL_H*/
