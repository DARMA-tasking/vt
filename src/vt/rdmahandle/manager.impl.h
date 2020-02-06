/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_MANAGER_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/rdmahandle/manager.h"

namespace vt { namespace rdma {
namespace impl {

struct HandleData {
  HandleData(HandleKey in_key, std::size_t in_size, int in_count)
    : key_(in_key),
      size_(in_size),
      count_(in_count)
  { }

  friend HandleData operator+(HandleData a1, HandleData const& a2) {
    vtAssertExpr(a1.key_.handle_ != vt::no_rdma_handle);
    vtAssertExpr(a1.key_.handle_ == a2.key_.handle_);
    vtAssertExpr(a1.key_.isObjGroup() == a2.key_.isObjGroup());
    a1.size_ += a2.size_;
    a1.count_ += a2.count_;
    return a1;
  }

  HandleKey key_;
  std::size_t size_ = 0;
  int count_ = 0;
};

template <typename T, HandleEnum E, typename ProxyT>
struct ConstructMsg : vt::collective::ReduceTMsg<HandleData> {
  ConstructMsg() = default;
  explicit ConstructMsg(HandleData&& data)
    : vt::collective::ReduceTMsg<HandleData>(std::move(data))
  { }
};

} /* end namespace impl */

template <typename T, HandleEnum E, typename ProxyT>
void Manager::finishMake(impl::ConstructMsg<T, E, ProxyT>* msg) {
  auto const& key = msg->getVal().key_;
  auto const& size = msg->getVal().size_;
  auto const& count = msg->getVal().count_;
  debug_print(
    rdma, node,
    "finishMake: handle={:x}, size={}, count={}\n",
    key.handle_, size, count
  );
  auto& entry = getEntry<T,E>(key);
  entry.allocateDataWindow();
}

template <typename T, HandleEnum E, typename ProxyT>
Handle<T, E> Manager::makeHandleCollectiveObjGroup(
  ProxyT proxy, std::size_t size, bool uniform_size
) {
  auto proxy_bits = proxy.getProxy();
  ElemType sub = 0;
  auto next_handle = ++cur_handle_obj_group_[proxy_bits];
  auto key = HandleKey{typename HandleKey::ObjGroupTag{}, proxy_bits, next_handle};
  auto han = Handle<T, E>{typename Handle<T, E>::NodeTagType{}, key, size};
  holder_<T,E>[key].template addHandle<ObjGroupProxyType>(key, sub, han, size, uniform_size);
  auto cb = vt::theCB()->makeBcast<
    Manager, impl::ConstructMsg<T, E, ProxyT>, &Manager::finishMake<T, E, ProxyT>
  >(proxy_);
  auto msg = vt::makeMessage<impl::ConstructMsg<T, E, ProxyT>>(
    impl::HandleData{key, size, 1}
  );
  proxy_.template reduce<vt::collective::PlusOp<impl::HandleData>>(msg.get(),cb);
  return han;
}

template <typename T, HandleEnum E>
void Manager::deleteHandleCollectiveObjGroup(Handle<T,E> const& han) {
  auto const key = han.key_;
  auto iter = holder_<T,E>.find(key);
  if (iter != holder_<T,E>.end()) {
    iter->second.deallocate();
    holder_<T,E>.erase(iter);
  }
}

template <
  typename T,
  HandleEnum E,
  typename ProxyT,
  typename IndexType
>
Handle<T, E> Manager::makeHandleCollectiveCollection(
  ProxyT proxy, IndexType range, std::size_t size
) {
  auto proxy_bits = proxy.getProxy();
  auto idx = proxy.getIndex();
  auto lin = static_cast<ElemType>(mapping::linearizeDenseIndexRowMajor(&idx, &range));
  auto next_handle = ++cur_handle_collection_[proxy_bits][lin];
  auto key = HandleKey{typename HandleKey::CollectionTag{}, proxy_bits, next_handle, size};
  auto han = Handle<T, E>(key, size);
  holder_<T,E>[key].template addHandle<VirtualProxyType>(key, lin, han);
  return han;
}

// For now, this is static. Really it should be part of the objgroup and the
// proxy should be available through the VT component
template <typename T, HandleEnum E>
Holder<T,E>& Manager::getEntry(HandleKey const& key) {
  vtAssertExpr((holder_<T,E>.find(key) != holder_<T,E>.end()));
  auto& entry = holder_<T,E>[key];
  return entry;
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_MANAGER_IMPL_H*/
