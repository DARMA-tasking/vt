/*
//@HEADER
// *****************************************************************************
//
//                              sub_handle.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H

#include "vt/config.h"
#include "vt/objgroup/manager.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename IndexT>
template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
void SubHandle<T,E,IndexT>::initialize(
  ProxyType in_proxy, bool in_is_static, IndexT in_range
) {
  map_han_ = auto_registry::makeAutoHandlerMap<IndexT, map_fn>();
  proxy_ = in_proxy;
  is_static_ = in_is_static;
  range_ = in_range;
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::makeSubHandles() {
  auto const total = totalLocalSize();
  auto const num_local = sub_handles_.size();
  debug_print(
    rdma, node,
    "total={}, num_local={}\n", total, num_local
  );
  for (uint64_t i = 0; i < sub_prefix_.size(); i++) {
    debug_print(
      rdma, node,
      "i={} prefix={} layout={}\n",
      i, sub_prefix_[i], sub_layout_[i]
    );
  }
  data_handle_ = proxy_.template makeHandleRDMA<T>(total, false);
  waitForHandleReady(data_handle_);
  loc_handle_ = proxy_.template makeHandleRDMA<uint64_t>((num_local + 1) * 3, false);
  waitForHandleReady(loc_handle_);
  vtAssertExpr(sub_prefix_.size() == num_local);
  vtAssertExpr(sub_layout_.size() == num_local);
  loc_handle_.modifyExclusive([&](uint64_t* t) {
    auto this_node = static_cast<uint64_t>(vt::theContext()->getNode());
    uint64_t i = 0;
    for (i = 0; i < num_local * 3; i += 3) {
      t[i+0] = linearize(sub_layout_[i/3]);
      t[i+1] = this_node;
      t[i+2] = sub_prefix_[i/3];
    }
    t[i+0] = 0;
    t[i+1] = this_node;
    t[i+2] = sub_handles_[sub_layout_[sub_layout_.size()-1]].size_ + sub_prefix_[(i-3)/3];
  });
  ready_ = true;
}

template <typename T, HandleEnum E, typename IndexT>
typename IndexT::DenseIndexType
SubHandle<T,E,IndexT>::linearize(IndexT idx) {
  debug_print(
    rdma, node,
    "linearize: idx={}, range={}\n",
    idx, range_
  );
  return vt::mapping::linearizeDenseIndexRowMajor(&idx, &range_);
}

template <typename T, HandleEnum E, typename IndexT>
NodeType SubHandle<T,E,IndexT>::getHomeNode(IndexT const& idx) {
  using BaseIdxType = vt::index::BaseIndex;
  auto fn = auto_registry::getHandlerMap(map_han_);
  auto const num_nodes = theContext()->getNumNodes();
  auto idx_p = idx;
  auto* cur = static_cast<BaseIdxType*>(&idx_p);
  auto* range = static_cast<BaseIdxType*>(&range_);
  return fn(cur, range, num_nodes);
}

template <typename T, HandleEnum E, typename IndexT>
IndexInfo SubHandle<T,E,IndexT>::fetchInfo(IndexT const& idx) {
  auto this_node = theContext()->getNode();
  auto home_node = getHomeNode(idx);
  auto home_size = loc_handle_.getSize(home_node);
  debug_print(
    rdma, node,
    "fetchInfo: idx={}, this_node={}, home_node={}, home_size={}\n",
    idx, this_node, home_node, home_size
  );
  auto lin_idx = linearize(idx);
  auto ptr = std::make_unique<uint64_t[]>(home_size);
  loc_handle_.get(home_node, &ptr[0], home_size, 0, Lock::Exclusive);
  for (uint64_t i = 0; i < home_size; i += 3) {
    debug_print_verbose(
      rdma, node,
      "fetchInfo: idx={}, this_node={}, home_node={}, ptr[{}]={},{},{}\n",
      idx, this_node, home_node, i, ptr[i+0], ptr[i+1], ptr[i+2]
    );
    if (ptr[i] == lin_idx) {
      return IndexInfo(ptr[i+1], ptr[i+2], ptr[i+6]-ptr[i+3]);
    }
  }
  vtAssert(false, "Could not find location info");
  return IndexInfo{};
}

template <typename T, HandleEnum E, typename IndexT>
IndexInfo SubHandle<T,E,IndexT>::resolveLocation(IndexT const& idx) {
  auto iter = sub_handles_.find(idx);
  if (iter != sub_handles_.end()) {
    auto const this_node = vt::theContext()->getNode();
    return IndexInfo(this_node, iter->second.offset_, iter->second.size_);
  } else {
    if (cache_.hasIndex(idx)) {
      return cache_.getInfo(idx);
    } else {
      auto info = fetchInfo(idx);
      cache_.saveInfo(idx, info);
      return info;
    }
  }
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::get(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.get(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
std::size_t SubHandle<T,E,IndexT>::getSize(IndexT const& idx, Lock) {
  if (uniform_size_) {
    return size_if_uniform_;
  } else {
    auto info = resolveLocation(idx);
    return info.getSize();
  }
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::rget(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.rget(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::rput(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.rput(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::put(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.put(node, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
RequestHolder SubHandle<T,E,IndexT>::raccum(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
  MPI_Op op
) {
  // @todo: make the location lookup async
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  return data_handle_.raccum(node, l, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
void SubHandle<T,E,IndexT>::accum(
  IndexT const& idx, Lock l, T* ptr, uint64_t len, int offset,
  MPI_Op op
) {
  auto info = resolveLocation(idx);
  auto node = info.getNode();
  auto chunk_offset = info.getOffset();
  data_handle_.accum(node, l, ptr, len, offset + chunk_offset, l);
}

template <typename T, HandleEnum E, typename IndexT>
template <typename Callable>
void SubHandle<T,E,IndexT>::access(
  IndexT idx, Lock l, Callable fn, uint64_t offset
) {
  auto iter = sub_handles_.find(idx);
  vtAssert(iter != sub_handles_.end(), "Index must be local here");
  auto local_offset  = iter->second.offset_;
  data_handle_.access(l, fn, offset + local_offset);
}

template <typename T, HandleEnum E, typename IndexT>
Handle<T, E, IndexT> SubHandle<T,E,IndexT>::addLocalIndex(
  IndexT index, uint64_t size
) {
  debug_print(
    rdma, node,
    "addLocalInddex: idx={}, size={}, range={}\n",
    index, size, range_
  );
  sub_layout_.push_back(index);
  if (sub_prefix_.size() > 0) {
    vtAssertExpr(sub_layout_.size() >= 2);
    // Compute a prefix as elements are added
    auto last_idx = sub_layout_[sub_layout_.size()-2];
    auto last_size_iter = sub_handles_.find(last_idx);
    vtAssertExpr(last_size_iter != sub_handles_.end());
    auto last_size = last_size_iter->second.size_;
    sub_prefix_.push_back(sub_prefix_[sub_prefix_.size()-1] + last_size);
  } else {
    sub_prefix_.push_back(0);
  }
  auto const offset = sub_prefix_[sub_prefix_.size()-1];
  sub_handles_[index] = SubInfo(size, offset);
  if (uniform_size_) {
    size_if_uniform_ = static_cast<std::size_t>(size);
  }
  return Handle<T,E,IndexT>{
    typename Handle<T,E,IndexT>::IndexTagType{},
    proxy_.getProxy(), index, size, 0
  };
}

template <typename T, HandleEnum E, typename IndexT>
uint64_t SubHandle<T,E,IndexT>::totalLocalSize() const {
  uint64_t total = 0;
  for (auto&& elm : sub_handles_) {
    total += elm.second.size_;
  }
  return total;
}

template <typename T, HandleEnum E, typename IndexT>
template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
/*static*/ typename SubHandle<T,E,IndexT>::ProxyType
SubHandle<T,E,IndexT>::construct(bool in_is_static, IndexT in_range) {
  auto proxy = vt::theObjGroup()->makeCollective<SubHandle<T,E,IndexT>>();
  proxy.get()->template initialize<map_fn>(proxy, in_is_static, in_range);
  return proxy;
}

template <typename T, HandleEnum E, typename IndexT>
template <typename U>
void SubHandle<T,E,IndexT>::waitForHandleReady(Handle<U,E> const& h) {
  do {
    vt::runScheduler();
  } while (not h.ready());
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_IMPL_H*/
