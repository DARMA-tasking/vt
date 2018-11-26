/*
//@HEADER
// ************************************************************************
//
//                          elm_holder.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/types/headers.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct ElementHolder {
  using VirtualPtrType = std::unique_ptr<CollectionBase<ColT,IndexT>>;

  ElementHolder(
    VirtualPtrType in_vc_ptr_, HandlerType const& in_han, IndexT const& idx
  );
  ElementHolder(ElementHolder&&) = default;

  virtual ~ElementHolder();

  typename VirtualPtrType::pointer getCollection() const;

  bool erased_ = false;
  VirtualPtrType vc_ptr_ = nullptr;
  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/elm_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H*/
