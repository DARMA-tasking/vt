/*
//@HEADER
// *****************************************************************************
//
//                             coll_constructors.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_CONSTRUCTOR_COLL_CONSTRUCTORS_H
#define INCLUDED_VT_VRT_COLLECTION_CONSTRUCTOR_COLL_CONSTRUCTORS_H

#include "vt/config.h"

#include "detector_headers.h"

#include <tuple>
#include <functional>

namespace vt { namespace vrt { namespace collection {

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsNoIndex {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(std::get<I>(*tup)...);
  }
};

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsIdxFst {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(idx,std::get<I>(*tup)...);
  }
};

template <
  typename ColT, typename IndexT, typename Tuple, typename RetT, size_t... I
>
struct DetectConsIdxSnd {
  RetT operator()(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...>
  ) {
    return std::make_unique<ColT>(std::get<I>(*tup)...,idx);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_CONSTRUCTOR_COLL_CONSTRUCTORS_H*/
