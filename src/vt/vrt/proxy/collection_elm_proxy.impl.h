/*
//@HEADER
// *****************************************************************************
//
//                           collection_proxy.impl.h
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

#if !defined INCLUDED_VT_VRT_PROXY_COLLECTION_ELM_PROXY_H
#define INCLUDED_VT_VRT_PROXY_COLLECTION_ELM_PROXY_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"

namespace vt { namespace vrt { namespace collection {

//Standard serialize, just pass along to base.
template <typename ColT, typename IndexT>
template <typename Ser>
void VrtElmProxy<ColT, IndexT>::serialize(DefaultSerializer<Ser>& s) {
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
}

//Checkpoint serialize, actually serialize the element itself.
template <typename ColT, typename IndexT>
template <typename Ser> 
void VrtElmProxy<ColT, IndexT>::serialize(CheckpointSerializer<Ser>& s) {
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  
  //Make sure proxies within the element don't also try recovering
  auto elm_serializer = checkpoint::withoutTrait<vt::vrt::CheckpointTrait>(s);

  auto local_elm_ptr = this->tryGetLocalPtr();
  if(local_elm_ptr != nullptr){
    local_elm_ptr | elm_serializer;
  } else {
    //The element is somewhere else so we'll need to request a migration to here.
    vtAssert(!s.isUnpacking(), "Must serialize elements from the node they are at");

    //Avoid delaying the serializer though, we want to enable asynchronous progress.
    std::unique_ptr<ColT> new_elm_ptr;
    new_elm_ptr | elm_serializer;

    auto ep = theCollection()->requestMigrateDeferred(*this, theContext()->getNode());

    theTerm()->addAction(ep, [*this, new_elm_ptr = std::move(new_elm_ptr)]{
      auto local_elm_ptr = *this.tryGetLocalPtr();
      assert(local_elm_ptr != nullptr);
      local_elm_ptr = std::move(new_elm_ptr);
    });
  }
}

//Deserialize without placing values into the runtime, 
//just return the element pointer.
template <typename ColT, typename IndexT>
template <typename Ser>
std::unique_ptr<ColT> 
VrtElmProxy<ColT, IndexT>::deserializeToElm(Ser& s) {
  //Still have to hit data in order.
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  
  //Make sure proxies within the element don't also try recovering
  auto elm_serializer = checkpoint::withoutTrait<vt::vrt::CheckpointTrait>(s);
  
  std::unique_ptr<ColT> elm;
  elm | elm_serializer;
  return elm;
}

}}} /* end namespace vt::vrt::collection */

