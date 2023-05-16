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

#if !defined INCLUDED_VT_VRT_PROXY_COLLECTION_ELM_PROXY_IMPL_H
#define INCLUDED_VT_VRT_PROXY_COLLECTION_ELM_PROXY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"

namespace vt { namespace vrt { namespace collection {

//Standard serialize, just pass along to base.
template <typename ColT, typename IndexT>
template <typename SerT, typename SerT::has_not_traits_t<CheckpointTrait>*>
void VrtElmProxy<ColT, IndexT>::serialize(SerT& s) {
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
}

//Checkpoint serialize, actually serialize the element itself.
template <typename ColT, typename IndexT>
template <typename SerT, typename SerT::has_traits_t<CheckpointTrait>*>
void VrtElmProxy<ColT, IndexT>::serialize(SerT& s) {
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  
  auto local_elm_ptr = this->tryGetLocalPtr();
  vtAssert(local_elm_ptr != nullptr || s.isUnpacking(), "Must serialize/size elements from the node they are at");
  
  //Traits for nested serialize/deserialize
  using CheckpointlessTraits = typename SerT::Traits::without<CheckpointTrait>::with<CheckpointInternalTrait>;
  
  //Weird nested serialization to enable asynchronous deserializing w/o changing semantics.
  if(!(s.isPacking() || s.isUnpacking())){
    int size = checkpoint::getSize(*local_elm_ptr);
    s.contiguousBytes(nullptr, 1, size);
  } else if(s.isPacking()){
    auto serialized_elm = checkpoint::serialize<CheckpointlessTraits>(*local_elm_ptr);
    int size = serialized_elm->getSize();
    s | size;
    s.contiguousBytes(serialized_elm->getBuffer(), 1, size);
  } else if(s.isUnpacking()){
    int size;
    s | size;

    auto buf = std::make_unique<char[]>(size);
    s.contiguousBytes(buf.get(), 1, size);
    
    if(local_elm_ptr != nullptr){
      checkpoint::deserializeInPlace<CheckpointlessTraits>(buf.get(), local_elm_ptr);
    } else {
      //The element is somewhere else so we'll need to request a migration to here.
      auto ep = theCollection()->requestMigrateDeferred(*this, theContext()->getNode());

      theTerm()->addActionUnique(ep, std::move([elm_proxy = *this, buffer = std::move(buf)]{
        auto elm_ptr = elm_proxy.tryGetLocalPtr();
        assert(elm_ptr != nullptr);
        checkpoint::deserializeInPlace<CheckpointlessTraits>(buffer.get(), elm_ptr);
      }));
    }
  }
}

//Deserialize without placing values into the runtime, 
//just return the element pointer.
template <typename ColT, typename IndexT>
template <typename SerT>
std::unique_ptr<ColT> 
VrtElmProxy<ColT, IndexT>::deserializeToElm(SerT& s) {
  //Still have to hit data in the same order.
  ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  
  int size;
  s | size;
  auto buf = std::make_unique<char[]>(size);
  s.contiguousBytes(buf.get(), 1, size);

  std::unique_ptr<ColT> elm(new ColT());
  
  using CheckpointlessTraits = typename SerT::Traits::without<CheckpointTrait>::with<CheckpointInternalTrait>;
  checkpoint::deserializeInPlace<CheckpointlessTraits>(buf.get(), elm.get());
  
  return elm;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_PROXY_COLLECTION_ELM_PROXY_IMPL_H*/
