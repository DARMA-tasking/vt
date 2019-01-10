/*
//@HEADER
// ************************************************************************
//
//                          mapped_node.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H
#define INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/vrt/proxy/base_collection_elm_proxy.h"
#include "vt/vrt/collection/mapped_node/mapped_node.impl.h"

#include <functional>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
void MappedNode<ColT,IndexT,BaseProxyT>::getMappedNode() const {
  auto const& idx = getIndex();
  auto col_proxy = this->getCollectionProxy();
  // auto elm_proxy = this->getElementProxy();
  // auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  theCollection()->getMappedNode(col_proxy,idx);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
void MappedNode<ColT,IndexT,BaseProxyT>::lookupLocation(FuncLocType fn) {
  vtAssert(0, "Not implemented yet");
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H*/
