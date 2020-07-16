/*
//@HEADER
// *****************************************************************************
//
//                          coll_constructors_deref.h
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

#if !defined INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H
#define INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.h"

#if vt_check_enabled(detector)
  #include "detector_headers.h"
#endif /*vt_check_enabled(detector)*/

#include <tuple>
#include <functional>

#if vt_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

struct DerefCons {
  template <typename ColT, typename IndexT, typename Tuple, typename... Args>
  static typename CollectionManager::VirtualPtrType<ColT, IndexT>
  derefTuple(
    VirtualElmCountType const& elms, IndexT const& idx, std::tuple<Args...>* tup
  );

  template <
    typename ColT, typename IndexT, typename Tuple, typename DispatcherT,
    size_t... I
  >
  static typename CollectionManager::VirtualPtrType<ColT, IndexT>
  expandSeq(
    VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
    std::index_sequence<I...> seq
  );
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/constructor/coll_constructors_deref.impl.h"

#endif /*vt_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_COLL_CONSTRUCTORS_DEREF_H*/
