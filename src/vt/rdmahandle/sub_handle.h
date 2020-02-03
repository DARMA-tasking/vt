/*
//@HEADER
// *****************************************************************************
//
//                                 sub_handle.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H
#define INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H

#include "vt/configs/types/types_type.h"
#include "vt/rdmahandle/handle.h"
#include "vt/rdmahandle/index_info.h"
#include "vt/rdmahandle/cache.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/topos/mapping/dense/dense.h"

#include <unordered_map>

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename IndexT>
struct SubHandle {
  using ProxyType = vt::objgroup::proxy::Proxy<SubHandle<T, E, IndexT>>;

  SubHandle() = default;

private:
  struct SubInfo {
    SubInfo() = default;
    SubInfo(std::size_t in_size, std::size_t in_offset)
      : size_(in_size),
        offset_(in_offset)
    { }
    std::size_t size_ = 0;
    std::size_t offset_ = 0;
  };

  struct LocInfo {
    union {
      struct {
        std::size_t idx;
        std::size_t node;
        std::size_t offset;
      } info;
      std::size_t arr[3];
    } u_;

    LocInfo() = default;
    // LocInfo(std::size_t in_node, std::size_t offset)
    //   : u_(
  };

public:
  template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
  void initialize(ProxyType in_proxy, bool in_is_static, IndexT in_range) {
    map_han_ = auto_registry::makeAutoHandlerMap<IndexT, map_fn>();
    proxy_ = in_proxy;
    is_static_ = in_is_static;
    range_ = in_range;
  }

  void makeSubHandles() {
    auto const total = totalLocalSize();
    auto const num_local = sub_handles_.size();
    debug_print(
      rdma, node,
      "total={}, num_local={}\n", total, num_local
    );
    for (std::size_t i = 0; i < sub_prefix_.size(); i++) {
      debug_print(
        rdma, node,
        "i={} prefix={} layout={}\n",
        i, sub_prefix_[i], sub_layout_[i]
      );
    }
    data_handle_ = theHandle()->makeHandleCollectiveObjGroup<T, E>(proxy_, total);
    waitForHandleReady(data_handle_);
    loc_handle_ = theHandle()->makeHandleCollectiveObjGroup<std::size_t, E>(proxy_, num_local * 3);
    waitForHandleReady(loc_handle_);
    vtAssertExpr(sub_prefix_.size() == num_local);
    vtAssertExpr(sub_layout_.size() == num_local);
    loc_handle_.modifyExclusive([&](std::size_t* t) {
      auto this_node = static_cast<size_t>(vt::theContext()->getNode());
      for (std::size_t i = 0; i < num_local * 3; i += 3) {
        t[i+0] = linearize(sub_layout_[i/3]);
        t[i+1] = this_node;
        t[i+2] = sub_prefix_[i];
      }
    });
  }

  typename IndexT::DenseIndexType linearize(IndexT idx) {
    debug_print(
      rdma, node,
      "linearize: idx={}, range={}\n",
      idx, range_
    );
    return vt::mapping::linearizeDenseIndexRowMajor(&idx, &range_);
  }

  NodeType getHomeNode(IndexT const& idx) {
    using BaseIdxType = vt::index::BaseIndex;
    auto fn = auto_registry::getHandlerMap(map_han_);
    auto const num_nodes = theContext()->getNumNodes();
    auto const cur = static_cast<BaseIdxType*>(&idx);
    auto const range = static_cast<BaseIdxType*>(&range_);
    return fn(cur, range, num_nodes);
  }

  IndexInfo fetchInfo(IndexT const& idx) {
    auto home_node = getHomeNode(idx);
    auto home_size = loc_handle_.getSize(home_node);
    auto lin_idx = linearize(idx);
    loc_handle_.addAction([=](std::size_t* b) {
      for (std::size_t i = 0; i < home_size; i += 3) {
        if (b[i] == lin_idx) {
          return IndexInfo(b[i+1], b[i+2]);
        }
      }
    });
    loc_handle_.get(home_node, home_size, 0);
  }

  IndexInfo resolveLocation(IndexT const& idx) {
    auto iter = sub_handles_.find(idx);
    if (iter != sub_handles_.end()) {
      auto const this_node = vt::theContext()->getNode();
      return IndexInfo(this_node, iter->second.offset_);
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

  void get(IndexT const& idx, Lock l, T* ptr, std::size_t len, int offset) {
    auto info = resolveLocation(idx);
    auto node = info.getNode();
    auto chunk_offset = info.getOffset();
    data_handle_.get(node, len, offset + chunk_offset, l);
  }

  template <typename Callable>
  void access(IndexT idx, Lock l, Callable fn, std::size_t offset) {
    auto iter = sub_handles_.find(idx);
    vtAssert(iter != sub_handles_.end(), "Index must be local here");
    auto local_offset  = iter->second.offset_;
    data_handle_.access(l, fn, offset + local_offset);
  }

  void addLocalIndex(IndexT index, std::size_t size) {
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
  }

  std::size_t totalLocalSize() const {
    std::size_t total = 0;
    for (auto&& elm : sub_handles_) {
      total += elm.second.size_;
    }
    return total;
  }

  template <mapping::ActiveMapTypedFnType<IndexT> map_fn>
  static ProxyType construct(bool in_is_static, IndexT in_range) {
    auto proxy = vt::theObjGroup()->makeCollective<SubHandle<T,E,IndexT>>();
    proxy.get()->template initialize<map_fn>(proxy, in_is_static, in_range);
    return proxy;
  }

private:
  template <typename U>
  void waitForHandleReady(Handle<U,E> const& h) {
    do {
      vt::runScheduler();
    } while (not h.ready());
  }

protected:
  ProxyType proxy_;
  bool is_static_ = false;
  vt::HandlerType map_han_ = 0;
  IndexT range_ = {};
  std::unordered_map<IndexT, SubInfo> sub_handles_;
  std::vector<IndexT> sub_layout_;
  std::vector<std::size_t> sub_prefix_;
  Handle<T, E> data_handle_;
  Handle<std::size_t, E> loc_handle_;
  Cache<IndexT> cache_;
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_SUB_HANDLE_H*/
